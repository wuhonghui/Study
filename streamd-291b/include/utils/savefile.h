#ifndef savefile_H
#define savefile_H

#ifdef __cplusplus
extern "C" {
#endif


void savejpg(unsigned char *addr, unsigned int size);

void savefile(char *filename, unsigned char *addr, unsigned int size);

#ifdef __cplusplus
}
#endif


#endif

