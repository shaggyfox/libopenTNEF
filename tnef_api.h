// API for external programs wanting to use TNEF decoding
//
#ifndef __TNEF_API__
#define __TNEF_API__
int TNEF_init( void );
int TNEF_decode( char *filename );
int TNEF_decode_tnef(const char *tnef_stream, int size);
int TNEF_set_filename_report_fn( int (*ptr_to_fn)(char *, char *));
int TNEF_set_verbosity( int level );
int TNEF_set_verbosity_contenttype( int level );
int TNEF_set_debug( int level );
int TNEF_set_path( char *path );
int TNEF_set_decode( int level );
int TNEF_set_save_attach_data(int (*handler)(char *, char*, unsigned int, void *), void*);
#endif
