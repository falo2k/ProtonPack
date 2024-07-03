/*  Glue functions for the minIni library, based on the Arduino libraries
 *
 *  By Bizan Nishimura, 2020
 *  This "glue file" is in the public domain. It is distributed without
 *  warranties or conditions of any kind, either express or implied.
 *
 *  (The Arduino libraries are copyright by Arduino Team and the others.
 *   They are licensed at their own terms.)
 */

/* map required file I/O types and functions to the standard C library */
#include <stdio.h>
#include <Arduino.h>
#include <SD.h>

#define INI_FILETYPE                    File

static int ini_openread(const char* filename, INI_FILETYPE *file)
{
    *file = SD.open(filename, FILE_READ);
    return ( (file->name())[0] != 0 );
}

static int ini_openwrite(const char* filename, INI_FILETYPE *file)
{
    *file = SD.open(filename, FILE_WRITE);
    return ( (file->name())[0] != 0 );
}

// #define ini_openrewrite(filename,file)   not defined

static int ini_close(INI_FILETYPE *file)
{
    file->close();
    return 1;
}

static int ini_read(char *buffer, int size, INI_FILETYPE *file)
{
    int i;
    
    for(i = 0; i < size-1; i++){
        if(file->available() > 0){
            char c = file->read();
            buffer[i] = c;
            if(c == '\n'){
                i++;
                break;
            }
        }else{
            break;
        }
    }
    buffer[i] = 0;
    
    return i;
}

static int ini_write(char *buffer, INI_FILETYPE *file)
{
    int size = strlen(buffer);
    return ( file->write(buffer, size) > 0);
}

static int ini_rename(const char *source, const char *dest)
{
    File srcFile = SD.open(source, FILE_READ);
    if(srcFile.name()[0] == 0) return 0;
    
    File dstFile = SD.open(dest, FILE_WRITE);
    if(dstFile.name()[0] == 0) return 0;
    
    const int BUFF_SIZE = 512;
    char buffer[BUFF_SIZE];
    int size;
    while((size = srcFile.available()) > 0){
        if(size > BUFF_SIZE) size = BUFF_SIZE;
        srcFile.read(buffer, size);
        dstFile.write(buffer, size);
    }
    
    srcFile.close();
    dstFile.close();
    
    return SD.remove(source);
}

#define ini_remove(filename)            ( SD.remove(filename) )

#define INI_FILEPOS                     long int
#define ini_tell(file,pos)              ( *(pos) = (file)->position() )
#define ini_seek(file,pos)              ( (file)->seek(*(pos)) )

/* for floating-point support, define additional types and functions */
#define INI_REAL                        float
#define ini_ftoa(string,value)          sprintf((string),"%f",(value))
#define ini_atof(string)                (INI_REAL)strtod((string),NULL)


