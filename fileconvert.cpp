#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>

typedef struct tagRow
{
    int  cols;
    int  colmax;
    int  rowlen;
    char delimeter;
    int  colslen[0];
}Row;

char *read_data_from_file(char *filepath, long *size);
long  write_data_to_file(char *filepath, char *data, long size);
Row  *analyze_columns_size(char *format);
void  handle_data_in_buffer(char *buffer, char *cache, Row *row, int rowcount);
char *handle_one_row_in_buffer(
        char *buffer, long size, char **columns, int *colnum);

#define DEFAULT_COLUMNS        1024

int main(int argc, char *argv[])
{
    char *sourceFile = (char*)"stdin.txt";
    char *targetFile = (char*)"stdout.txt";
    char  delimeter = ',';
    char *format = NULL;

    long  filesize = 0;
    long  cachesize = 0;
    int   rowcount = 0;
    char *buffer = NULL;
    char *cache = NULL;
    Row  *row;

    switch(argc)
    {
    case 5:
        delimeter = (char)argv[4][0];
    case 4:
        targetFile = argv[3];
    case 3:
        sourceFile = argv[2];
    case 2:
        format = argv[1];
        printf("[INFO] the source file [%s], target file [%s], format [%s], "
               "delimeter [%c]\n", sourceFile, targetFile, format, delimeter);
        break;

    default:
        printf("The command syntax as following:\n"
               "%s format [source_file [target_file [delimeter]]]\n"
               "\tformat: \"number{,number}+\"\n"
               "\tdelimeter: default is \",\"\n",
               argv[0]);
        exit(1);
        break;
    }

    buffer = read_data_from_file(sourceFile, &filesize);
    if (buffer == NULL)
        goto error;

    row = analyze_columns_size(format);
    row->delimeter = delimeter;

    rowcount = filesize/row->rowlen;
    cachesize = filesize + rowcount * (row->cols - 1);
    cache = (char*)malloc(cachesize);

    if (filesize < row->rowlen)
    {
        cache[0] = '\n';
        cachesize = 1;
    }

    handle_data_in_buffer(buffer, cache, row, rowcount);

    write_data_to_file(targetFile, cache, cachesize);

error:
    if (buffer != NULL)
    {
        free(buffer);
        buffer = NULL;
    }

    if (cache != NULL)
    {
        free(cache);
        cache = NULL;
    }

    return 0;
}

Row *analyze_columns_size(char *format)
{
    int   size = sizeof(Row) + (DEFAULT_COLUMNS + 1) * sizeof(int);
    Row  *row = NULL;
    char *start = format;
    char *cur = format;
    int   collen = 0;

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
            if (row->cols >= row->colmax)
            {
                int newsize = sizeof(Row) + (2 * row->colmax + 1) * sizeof(int);
                Row *newrow = (Row*)malloc(newsize);
                size = sizeof(Row) + row->colmax * sizeof(int); 
                row->colmax *= 2;
                memcpy(newrow, row, size);
            }
        
            collen = atoi(start);
            row->colslen[row->cols] = collen;
            row->rowlen += collen;
            row->cols++;
            start = cur;
        }
        break;
        
        default:
            break;
        }
    }

    collen = atoi(start);
    row->colslen[row->cols] = collen;
    row->rowlen += collen;
    row->cols++;

    printf("[INFO] columns [%d] total rowlen [%d]\n", row->cols, row->rowlen);

    return row;
}

void handle_one_row_in_buffer(char *buffer, char *cache, Row *row)
{
    char *curbuf = buffer;
    char *curcache = cache;
    char  delimeter = row->delimeter;
    
    int   col = 0;
    int   collen = 0;
    int  *colslen = row->colslen;
    
    for (col = 0; col < row->cols; col++)
    {
        collen = colslen[col];
        memcpy(curcache, curbuf, collen);
        curcache[collen] = delimeter;
        curbuf += collen;
        curcache += collen + 1;
    }
    cache[row->rowlen + row->cols - 1] = '\n';
}

void handle_data_in_buffer(char *buffer, char *cache, Row *row, int rowcount)
{
    int   rowid = 0;
    int   rowlen = row->rowlen + 1;
    int   cols = row->cols - 1;
    char *curbuf = buffer;
    char *curcache = cache;

    printf("[INFO] handle the buffer [%p] start, rows [%d], cache [%p]\n", 
           buffer, rowcount, cache);
    for (rowid = 0; rowid < rowcount; rowid++)
    {
        handle_one_row_in_buffer(curbuf, curcache, row);
#ifdef _DEBUG_
        printf("[INFO] row [%d], buffer [%p], cache [%p]\n", 
               rowid, curbuf, curcache);
#endif
        curbuf += rowlen;
        curcache += rowlen + cols;
    }

    printf("[INFO] handle the buffer [%p] succeed, rows [%d], columns[%d]\n", 
           buffer, rowcount, row->cols);
}

char *read_data_from_file(char *filepath, long *size)
{
    FILE *stream = NULL;
    long  filesize = 0;
    char *buffer = NULL;

    if (size != NULL)
        *size = 0;

    printf("[INFO] open file [%s]\n", filepath);
    if((stream = fopen(filepath,"r")) == NULL)
    {
        printf("[ERROR] open file [%s] fail, errno:%d\n", 
               filepath, errno);
        goto error;
    }

    printf("[INFO] get file [%s] size\n", filepath);
    if (fseek(stream, 0, SEEK_END))
    {
        printf("[ERROR] file [%s] fseek end with error [%d]\n", filepath, errno);
        goto error;
    }
    filesize = ftell(stream);
    if (fseek(stream, 0, SEEK_SET))
    {
        printf("[ERROR] file [%s] fseek head with error [%d]\n", filepath, errno);
        goto error;
    }
    printf("[INFO] the file [%s] size [%ld]\n", filepath, filesize);

    printf("[INFO] malloc memory buffer size [%ld]\n", filesize);
    buffer = (char*)malloc(filesize + 10);
    printf("[INFO] the memory buffer [%p——%p]\n", buffer, buffer + filesize);

    printf("[INFO] read data from file [%s] size [%ld]\n", 
           filepath, filesize);
    if (fread(buffer, filesize, 1, stream) != 1)
    {
        printf("[INFO] read file [%s] with error[%d]\n", 
               filepath, errno);
        goto error;
    }

    printf("[INFO] read file [%s] is closed\n", filepath);
    fclose(stream);
    if (size != NULL)
        *size = filesize;

    return buffer;

error:
    if (buffer != NULL)
    {
        free(buffer);
        buffer = NULL;
    }

    if (stream != NULL)
    {
        printf("[INFO] read file [%s] is closed\n", filepath);
        fclose(stream);
    }

    return NULL;
}

long  write_data_to_file(char *filepath, char *data, long size)
{
    FILE *stream = NULL;

    printf("[INFO] open file [%s] to write\n", filepath);
    if((stream = fopen(filepath,"w")) == NULL)
    {
        printf("[ERROR] open file [%s] to write fail, errno:%d\n", 
               filepath, errno);
        return -1;
    }

    printf("[INFO] write data [%p] to file [%s] size [%ld]\n", 
           data, filepath, size);
    if (fwrite(data, size, 1, stream) != 1)
    {
        printf("[INFO] write file [%s] with error[%d]\n", 
               filepath, errno);
        goto error;
    }

    printf("[INFO] write file [%s] is closed\n", filepath);
    fclose(stream);

    return size;

error:
    if (stream != NULL)
    {
        printf("[INFO] write file [%s] is closed\n", filepath);
        fclose(stream);
    }

    return 0;
}
