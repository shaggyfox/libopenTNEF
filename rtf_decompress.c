#include <sys/types.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include "rtf_decompress.h"

struct read_buff {
  int fd;
  uint8_t *buff;
  uint32_t size;
  uint32_t pos;
};

static int read_u32_le(struct read_buff *buff, uint32_t *ret)
{
  if (buff->pos + 4 > buff->size) {
    return -1;
  }
  *ret = 0;
  uint8_t *bytes = buff->buff + buff->pos;
  for (int i = 0; i < 4; ++i) {
    (*ret) += ((uint32_t)bytes[i]) << (8 * i);
  }
  buff->pos += 4;
  return 0;
}

static int read_u16_be(struct read_buff *buff, uint16_t *ret)
{
  if (buff->pos + 2 > buff->size) {
    return -1;
  }
  *ret = 0;
  uint8_t *bytes = buff->buff + buff->pos;
  for (int i = 0; i < 2; ++i) {
    *ret += ((uint32_t)bytes[i]) << (8 * (1 - i));
  }
  buff->pos += 2;
  return 0;
}

static int read_u8(struct read_buff *buff, uint8_t *ret)
{
  if (buff->pos >= buff->size) {
    return -1;
  }
  *ret = buff->buff[buff->pos++];
  return 0;
}

static uint16_t dict_ref_offset(uint16_t v)
{
  return v >> 4;
}
static uint8_t dict_ref_length(uint16_t v)
{
  return (v & 0xf) + 2;
}

struct rtfc_header {
  uint32_t compsize;
  uint32_t rawsize;
  uint32_t comptype;
  uint32_t crc;
};


static int read_header(struct read_buff *buff, struct rtfc_header* header)
{
  if (read_u32_le(buff, &header->compsize)) {
    return -1;
  }
  if (read_u32_le(buff, &header->rawsize)) {
    return -1;
  }
  if (read_u32_le(buff, &header->comptype)) {
    return -1;
  }
  if (read_u32_le(buff, &header->crc)) {
    return -1;
  }
  return 0;
}

static int intern_rtf_decompress(struct read_buff* in_buff, int (*out_cb)(uint8_t, void *, uint32_t), void *cb_data) {


  /* init dict */
  uint8_t dictionary[4096] = "{\\rtf1\\ansi\\mac\\deff0\\deftab720{\\fonttbl;}"
                             "{\\f0\\fnil \\froman \\fswiss \\fmodern \\fscript"
                             " \\fdecor MS Sans SerifSymbolArialTimes New Roman"
                             "Courier{\\colortbl\\red0\\green0\\blue0\r\n\\par \\"
                             "pard\\plain\\f0\\fs20\\b\\i\\u\\tab\\tx";
  uint16_t dict_r_offset = 0;
  uint16_t dict_w_offset = 207;

  struct rtfc_header head={0};
  if (read_header(in_buff, &head)) {
    printf("error reading header\n");
    return -1;
  }
  printf("compsize: %u\n"
         "rawsize: %u\n", head.compsize, head.rawsize);

  out_cb(0, cb_data, head.rawsize);
  int running = 1;
  while(running) {
    uint8_t control;
    if (-1 == read_u8(in_buff, &control)) {
      printf("error reading from buffer\n");
      break;
    }
    for(int i = 1; i < 256; i *=2) {
      uint8_t c;
      if (0 == (control & i)) {
        /* read data from stream */
        if (read_u8(in_buff, &c)) {
          printf("error reading from buffer\n");
          running = 0;
          break;
        }
        out_cb(c, cb_data, 0);
        dictionary[dict_w_offset] = c;
        dict_w_offset = (dict_w_offset + 1) % 4096;
      } else {
        // read data from dictionary reference
        uint16_t ref = 0;
        if (read_u16_be(in_buff, &ref)) {
          printf("error reading from buffer\n");
          running = 0;
          break;
        }
        uint8_t len = dict_ref_length(ref);
        uint16_t off = dict_ref_offset(ref);
        dict_r_offset = off;
        if (off == dict_w_offset) {
          running = 0;
          break;
        }
        while(len--) {
          c = dictionary[dict_r_offset];
          out_cb(c, cb_data, 0);
          dict_r_offset = (dict_r_offset + 1) % 4096;
          dictionary[dict_w_offset] = c;
          dict_w_offset = (dict_w_offset + 1) % 4096;
        }
      }
    }
  }
  return 0;
}

static int out_cb(uint8_t c, void *data, uint32_t length)
{
  if (!length) {
    int fd = (size_t)data;
    return write(fd, &c, 1);
  }
  return 0;
}

int rtf_decompress_to_file(char *data, unsigned int len, const char *filename)
{
  struct read_buff buff = {0};
  buff.buff = (uint8_t *)data;
  buff.size = (uint32_t)len;
  int out_fd = open(filename, O_CREAT|O_TRUNC|O_WRONLY, 0666);
  intern_rtf_decompress(&buff, out_cb, (void*)(size_t)out_fd);
  close(out_fd);
  return 0;
}
/*
static int open_buff(const char *filename, struct read_buff *buff)
{
  struct stat st;
  buff->fd = open(filename, O_RDONLY);
  if (-1 == buff->fd) {
    printf("error open file\n");
    return -1;
  }
  fstat(buff->fd, &st);
  buff->size = st.st_size;
  buff->buff = mmap(NULL, st.st_size, PROT_READ, MAP_PRIVATE, buff->fd, 0);
  buff->pos = 0;
  return 0;
}

static int close_buff(struct read_buff *buff)
{
  munmap(buff->buff, buff->size);
  close(buff->fd);
  return 0;
}

int main() {
  struct read_buff buff;

  if (open_buff("XAM_0.rtf", &buff)) {
    return -1;
  }

  int out_fd = open("OUT.rtf", O_CREAT|O_TRUNC|O_WRONLY, 0666);
  intern_rtf_decompress(&buff, out_cb, (void*)(size_t)out_fd);
  close(out_fd);

  close_buff(&buff);
  return 0;
}
*/
