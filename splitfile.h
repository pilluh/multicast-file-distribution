/* 
 * File:   splitfile.h
 * Author: pilluh
 *
 * Created on 28 décembre 2015, 15:06
 */

#ifndef SPLITFILE_H
#define SPLITFILE_H

#include "types.h"  /* tBlockSize */

#ifdef __cplusplus
extern "C" {
#endif

void createOutputDir(const char* const outputDir);
void resetOuputDir(const char* const outputDir);
void splitFile(const char* const fileName,
               const char* const outputDir,
               const tBlockSize blockSize);

#ifdef __cplusplus
}
#endif

#endif /* SPLITFILE_H */

