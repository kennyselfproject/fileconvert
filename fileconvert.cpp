#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>

typedef struct tagRow
{
    int  colcnt;
    int  colmax;
    int  rowsize;
    char delimeter;
    int  colslen[0];
}Row;

typedef struct tagBuffer
{
    size_t size;
    size_t maxsize;
    long   rowsize;
    long   rowcnt;
    char   data[0];
}Buffer;


// file operator
FILE   *file_open(char *filepath, char *mode);
long    file_read(FILE *stream, Buffer *buffer);
long    file_write(FILE *stream, Buffer *buffer);
size_t  file_get_size(FILE *stream);
void    file_close(FILE *stream);

Row    *analyze_columns_size(char *format);
Buffer *alloc_buffer(Row *row, size_t size);

void    convert_buffer_data(Buffer *sourceBuf, Buffer *targetBuf, Row *row);
void    convert_one_row(char *source, char *target, Row *row);

#define DEFAULT_COLUMNS        1024
#define BUFFER_SIZE            (1024*1024*1024L)

int main(int argc, char *argv[])
{
    char   *sourceFile = (char*)"stdin.txt";
    char   *targetFile = (char*)"stdout.txt";
    char    targetPath[128];
    FILE   *source = NULL;
    FILE   *target = NULL;

    char    delimeter = ',';
    char   *format = NULL;

    size_t  filesize = 0;
    size_t  bufsize = BUFFER_SIZE;
    int     batch = 0;
    int     batchcnt = 0;
    Buffer *sourceBuf = NULL;
    Buffer *targetBuf = NULL;
    Row    *row = NULL;

    switch(argc)
    {
    case 6:
        bufsize = atoi(argv[5]) * BUFFER_SIZE;
        if (bufsize < BUFFER_SIZE)
            bufsize = BUFFER_SIZE;
    case 5:
        delimeter = (char)argv[4][0];
    case 4:
        targetFile = argv[3];
    case 3:
        sourceFile = argv[2];
    case 2:
        format = argv[1];
        printf("[INFO] the source file [%s], target file [%s], format [%s], "
               "delimeter [%c] buffer size [%lld]\n", 
               sourceFile, targetFile, format, delimeter, bufsize);
        break;

    default:
        printf("The command syntax as following:\n"
               "%s format [source_file [target_file [delimeter [buffersize]]]]\n"
               "\tformat: \"number{,number}+\"\n"
               "\tdelimeter: default is \",\"\n"
               "\tbuffersize: n(GB)"
               "example: \n\t%s \"1,2\" source.txt target.txt \"|\"\n",
               argv[0], argv[0]);
        exit(1);
        break;
    }

    row = analyze_columns_size(format);
    row->delimeter = delimeter;

    printf("[INFO] file [%s], columns [%s], format [%s], "
           "delimeter [%c]\n", sourceFile, targetFile, format, delimeter);


    source = file_open(sourceFile, (char*)"r");
    if (source == NULL)
        goto error;

    filesize = file_get_size(source);
    
    if (filesize < bufsize)
        bufsize = filesize;
    sourceBuf = alloc_buffer(row, bufsize);
    sourceBuf->rowcnt = bufsize/sourceBuf->rowsize;
    batchcnt = (filesize + bufsize - 1)/bufsize;

    printf("[INFO] file [%s] size [%lld], batch count [%d]\n",
           sourceFile, filesize, batchcnt);
    
    targetBuf = alloc_buffer(row, 
                             bufsize + (row->colcnt-1) * sourceBuf->rowcnt);
    targetBuf->rowsize += (row->colcnt - 1);
    targetBuf->rowcnt = sourceBuf->rowcnt;

    while(filesize >= row->rowsize)
    {
        sourceBuf->size = sourceBuf->rowcnt * sourceBuf->rowsize;

        if (filesize < sourceBuf->size)
            sourceBuf->size = filesize;
        filesize -= sourceBuf->size;

        file_read(source, sourceBuf);
        printf("[INFO] read data start batch [%d], batch size [%lld]\n",
               batch++, sourceBuf->size);

        convert_buffer_data(sourceBuf, targetBuf, row);

        if (batchcnt > 1)
            sprintf(targetPath, "%s.%d", targetFile, batch);
        else 
            sprintf(targetPath, "%s", targetFile);
        target = file_open(targetPath, (char*)"w");
        if (target == NULL)
            goto error;

        file_write(target, targetBuf);
    }

error:
    if (source != NULL)
    {
        file_close(source);
        source = NULL;
    }

    if (target != NULL)
    {
        file_close(target);
        target = NULL;
    }

    if (sourceBuf != NULL)
    {
        free(sourceBuf);
        sourceBuf = NULL;
    }

    if (targetBuf != NULL)
    {
        free(targetBuf);
        targetBuf = NULL;
    }

    printf("[INFO] convert file [%s] finished, the target file [%s]\n\n",
           sourceFile, targetFile);


    return 0;
}

Row *analyze_columns_size(char *format)
{
    int   size = sizeof(Row) + (DEFAULT_COLUMNS + 1) * sizeof(int);
    int   collen = 0;
    Row  *row = NULL;
    char *start = format;
    char *cur = format;

    printf("[INFO] column format [%s]\n", format);

    row = (Row*)malloc(size);
    memset(row, 0, sizeof(Row));
    row->colmax = DEFAULT_COLUMNS;

    while(*cur)
    {
        switch (*cur++)
        {
        case ',':
        {
            if (row->colcnt >= row->colmax)
            {
                int newsize = sizeof(Row) + (2 * row->colmax + 1) * sizeof(int);
                Row *newrow = (Row*)malloc(newsize);
                size = sizeof(Row) + row->colmax * sizeof(int); 
                row->colmax *= 2;
                memcpy(newrow, row, size);
            }
        
            collen = atoi(start);
            row->colslen[row->colcnt] = collen;
            row->rowsize += collen;
            row->colcnt++;
            start = cur;
        }
        break;
        
        default:
            break;
        }
    }

    collen = atoi(start);
    row->colslen[row->colcnt] = collen;
    row->rowsize += collen + 1;
    row->colcnt++;

    printf("[INFO] columns [%d] total rowsize [%d]\n", row->colcnt, row->rowsize);

    return row;
}

Buffer *alloc_buffer(Row *row, size_t size)
{
    Buffer *sourceBuf = (Buffer*)malloc(sizeof(Buffer) + size);

    sourceBuf->size = 0;
    sourceBuf->maxsize = size;
    sourceBuf->rowsize = row->rowsize;
    sourceBuf->rowcnt = -1;

    return sourceBuf;
}

void convert_one_row(char *source, char *target, Row *row)
{
    char *cursource = source;
    char *curtarget = target;
    char  delimeter = row->delimeter;
    
    int   col = 0;
    int   collen = 0;
    int  *colslen = row->colslen;
    
    for (col = 0; col < row->colcnt; col++)
    {
        collen = colslen[col];
        memcpy(curtarget, cursource, collen);
        curtarget[collen] = delimeter;
        cursource += collen;
        curtarget += collen + 1;
    }

    target[row->rowsize + row->colcnt - 2] = '\n';
}

void convert_buffer_data(Buffer *sourceBuf, Buffer *targetBuf, Row *row)
{
    int   rowid = 0;
    int   rowsize = row->rowsize;
    int   colcnt = row->colcnt - 1;
    int   rowcnt = sourceBuf->size/rowsize;
    char *source = sourceBuf->data;
    char *target = targetBuf->data;

    printf("[INFO] handle the buffer [%p] start, rows [%d], target buffer [%p]\n", 
           sourceBuf, rowcnt, targetBuf);

    targetBuf->size = 0;
    for (rowid = 0; rowid < rowcnt; rowid++)
    {
        convert_one_row(source, target, row);

        if (*(source + rowsize - 1) != '\n')
            printf("[ERROR] row [%d], buffer [%s], target buffer [%p]\n", 
                   rowid, source, target);
        source += rowsize;
        target += rowsize + colcnt;
    }

    targetBuf->size = (target - targetBuf->data);

    printf("[INFO] handle the buffer [%p] succeed, rows [%ld], columns[%d]\n", 
           sourceBuf, rowid, row->colcnt);
}

size_t file_get_size(FILE *stream)
{
    size_t  size = 0;

    if (fseek(stream, 0, SEEK_END))
    {
        printf("[ERROR] file [%p] fseek end with error [%d]\n", stream, errno);
        return -1;
    }

    size = ftell(stream);
    if (fseek(stream, 0, SEEK_SET))
    {
        printf("[ERROR] file [%p] fseek head with error [%d]\n", stream, errno);
        return -1;
    }

    return size;
}

FILE *file_open(char *filepath, char *mode)
{
    FILE *stream = NULL;

    printf("[INFO] open file [%s]\n", filepath);
    if((stream = fopen(filepath, mode)) == NULL)
    {
        printf("[ERROR] open file [%s] fail, errno:%d\n", 
               filepath, errno);
    }

    return stream;
}

long file_read(FILE *stream, Buffer *buffer)
{
    long size = 0; 

#ifdef _DEBUG_
    printf("[INFO] read data from file [%p] size [%ld]\n", stream, size);
#endif

    size = fread(buffer->data, buffer->size, 1, stream);
    if (size != 1)
    {
        printf("[ERROR] read file [%p] with error[%d]\n", stream, errno);
    }

    return size;
}

long  file_write(FILE *stream, Buffer *buffer)
{
    long size = 0;

#ifdef _DEBUG_
    printf("[INFO] write data [%p] to file [%p], size [%ld]\n", 
           buffer->data, stream, buffer->size);
#endif

    size = fwrite(buffer->data, buffer->size, 1, stream);
    if (size != 1)
    {
        printf("[ERROR] write file [%p] with error[%d]\n", stream, errno);
    }

    return size;
}

void  file_close(FILE *stream)
{
    if (stream != NULL)
    {
#ifdef _DEBUG_
        printf("[INFO] file [%p] is closed\n", stream);
#endif
        fclose(stream);
    }
}
