#include <string.h>
#include <ctype.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdio.h>

void* memcpy(void* dest, const void* src, size_t n) {
    char* d = dest; const char* s = src;
    while(n--) *d++ = *s++;
    return dest;
}
void* memset(void* s, int c, size_t n) {
    unsigned char* p = s;
    while(n--) *p++ = (unsigned char)c;
    return s;
}
int memcmp(const void* s1, const void* s2, size_t n) {
    const unsigned char* p1 = s1; const unsigned char* p2 = s2;
    while(n--) { if (*p1 != *p2) return *p1 - *p2; p1++; p2++; }
    return 0;
}
size_t strlen(const char* str) { size_t l=0; while(str[l]) l++; return l; }
int strcmp(const char* s1, const char* s2) {
    while(*s1 && (*s1 == *s2)) { s1++; s2++; }
    return *(const unsigned char*)s1 - *(const unsigned char*)s2;
}
int strncmp(const char* s1, const char* s2, size_t n) {
    while(n-- && *s1 && (*s1 == *s2)) { s1++; s2++; }
    if (n == (size_t)-1) return 0;
    return *(const unsigned char*)s1 - *(const unsigned char*)s2;
}
char* strcpy(char* dest, const char* src) { char* d=dest; while((*d++ = *src++)); return dest; }
char* strncpy(char* dest, const char* src, size_t n) {
    size_t i;
    for(i=0; i<n && src[i]; i++) dest[i] = src[i];
    for(; i<n; i++) dest[i] = 0;
    return dest;
}
char* strcat(char* dest, const char* src) { strcpy(dest + strlen(dest), src); return dest; }
char* strchr(const char* s, int c) { while(*s) { if (*s==c) return (char*)s; s++; } return c==0 ? (char*)s : 0; }
int toupper(int c) { return (c>='a' && c<='z') ? c-0x20 : c; }
int tolower(int c) { return (c>='A' && c<='Z') ? c+0x20 : c; }
int isspace(int c) { return c==' '||c=='\t'||c=='\n'||c=='\r'; }
int isdigit(int c) { return c>='0' && c<='9'; }
int isalpha(int c) { return (c>='a'&&c<='z') || (c>='A'&&c<='Z'); }
int atoi(const char* str) {
    int res=0, sign=1;
    while(isspace(*str)) str++;
    if (*str=='-') { sign=-1; str++; } else if (*str=='+') str++;
    while(isdigit(*str)) { res=res*10 + (*str-'0'); str++; }
    return res*sign;
}
int abs(int j) { return j<0 ? -j : j; }
int strcasecmp(const char* s1, const char* s2) {
    while(*s1 && (tolower(*s1) == tolower(*s2))) { s1++; s2++; }
    return tolower(*s1) - tolower(*s2);
}
int strncasecmp(const char* s1, const char* s2, size_t n) {
    while(n-- && *s1 && (tolower(*s1) == tolower(*s2))) { s1++; s2++; }
    if (n == (size_t)-1) return 0;
    return tolower(*s1) - tolower(*s2);
}
void itoa(int val, char* buf, int base) {
    if (val == 0) { buf[0] = '0'; buf[1] = 0; return; }
    int i = 0, sign = 0;
    if (val < 0 && base == 10) { sign = 1; val = -val; }
    while (val != 0) { int rem = val % base; buf[i++] = (rem > 9) ? (rem - 10) + 'a' : rem + '0'; val /= base; }
    if (sign) buf[i++] = '-';
    buf[i] = 0;
    int start = 0, end = i - 1;
    while (start < end) { char t = buf[start]; buf[start] = buf[end]; buf[end] = t; start++; end--; }
}
int vsnprintf(char* str, size_t size, const char* format, va_list ap) {
    if (size == 0) return 0;
    size_t i = 0;
    while (*format && i < size - 1) {
        if (*format == '%') {
            format++;
            // Parse flags
            int zero_pad = 0, left_align = 0;
            while (*format == '0' || *format == '-' || *format == '+' || *format == ' ') {
                if (*format == '0') zero_pad = 1;
                if (*format == '-') left_align = 1;
                format++;
            }
            // Parse width
            int width = 0;
            while (*format >= '0' && *format <= '9') {
                width = width * 10 + (*format - '0');
                format++;
            }
            // Parse precision
            int has_prec = 0, prec = 0;
            if (*format == '.') {
                has_prec = 1;
                format++;
                while (*format >= '0' && *format <= '9') {
                    prec = prec * 10 + (*format - '0');
                    format++;
                }
            }
            // Parse length modifier (ignore but consume)
            if (*format == 'l') { format++; if (*format == 'l') format++; }
            else if (*format == 'h') { format++; if (*format == 'h') format++; }
            else if (*format == 'z') { format++; }
            // Parse specifier
            if (*format == 'd' || *format == 'i') {
                int val = va_arg(ap, int);
                char buf[32];
                int neg = 0;
                unsigned int uval;
                if (val < 0) { neg = 1; uval = (unsigned int)(-(val + 1)) + 1; } else { uval = val; }
                int bi = 0;
                if (uval == 0) { buf[bi++] = '0'; }
                else { while (uval) { buf[bi++] = '0' + (uval % 10); uval /= 10; } }
                // Reverse
                for (int s=0, e=bi-1; s<e; s++, e--) { char t=buf[s]; buf[s]=buf[e]; buf[e]=t; }
                // Apply precision (minimum digits)
                int min_digits = has_prec ? prec : 1;
                int pad_zeros = (min_digits > bi) ? min_digits - bi : 0;
                int total_len = neg + pad_zeros + bi;
                // Apply width padding (with zero_pad if no precision)
                int pad_spaces = (width > total_len) ? width - total_len : 0;
                if (zero_pad && !has_prec && !left_align) {
                    pad_zeros += pad_spaces;
                    pad_spaces = 0;
                    total_len = neg + pad_zeros + bi;
                }
                if (!left_align) { for (int p=0; p<pad_spaces && i<size-1; p++) str[i++] = ' '; }
                if (neg && i<size-1) str[i++] = '-';
                for (int p=0; p<pad_zeros && i<size-1; p++) str[i++] = '0';
                for (int p=0; p<bi && i<size-1; p++) str[i++] = buf[p];
                if (left_align) { for (int p=0; p<pad_spaces && i<size-1; p++) str[i++] = ' '; }
            } else if (*format == 'u') {
                unsigned int val = va_arg(ap, unsigned int);
                char buf[32]; int bi = 0;
                if (val == 0) { buf[bi++] = '0'; }
                else { while (val) { buf[bi++] = '0' + (val % 10); val /= 10; } }
                for (int s=0, e=bi-1; s<e; s++, e--) { char t=buf[s]; buf[s]=buf[e]; buf[e]=t; }
                int pad = (width > bi) ? width - bi : 0;
                char pc = zero_pad ? '0' : ' ';
                if (!left_align) { for (int p=0; p<pad && i<size-1; p++) str[i++] = pc; }
                for (int p=0; p<bi && i<size-1; p++) str[i++] = buf[p];
                if (left_align) { for (int p=0; p<pad && i<size-1; p++) str[i++] = ' '; }
            } else if (*format == 'x' || *format == 'X') {
                unsigned int val = va_arg(ap, unsigned int);
                const char* digits = (*format == 'x') ? "0123456789abcdef" : "0123456789ABCDEF";
                char buf[32]; int bi = 0;
                if (val == 0) { buf[bi++] = '0'; }
                else { while (val) { buf[bi++] = digits[val & 0xF]; val >>= 4; } }
                for (int s=0, e=bi-1; s<e; s++, e--) { char t=buf[s]; buf[s]=buf[e]; buf[e]=t; }
                int min_digits = has_prec ? prec : 1;
                int pad_zeros = (min_digits > bi) ? min_digits - bi : 0;
                int total_len = pad_zeros + bi;
                int pad_spaces = (width > total_len) ? width - total_len : 0;
                if (zero_pad && !has_prec && !left_align) { pad_zeros += pad_spaces; pad_spaces = 0; }
                if (!left_align) { for (int p=0; p<pad_spaces && i<size-1; p++) str[i++] = ' '; }
                for (int p=0; p<pad_zeros && i<size-1; p++) str[i++] = '0';
                for (int p=0; p<bi && i<size-1; p++) str[i++] = buf[p];
                if (left_align) { for (int p=0; p<pad_spaces && i<size-1; p++) str[i++] = ' '; }
            } else if (*format == 's') {
                const char* s = va_arg(ap, const char*);
                if (!s) s = "(null)";
                int slen = 0; while (s[slen]) slen++;
                if (has_prec && prec < slen) slen = prec;
                int pad = (width > slen) ? width - slen : 0;
                if (!left_align) { for (int p=0; p<pad && i<size-1; p++) str[i++] = ' '; }
                for (int p=0; p<slen && i<size-1; p++) str[i++] = s[p];
                if (left_align) { for (int p=0; p<pad && i<size-1; p++) str[i++] = ' '; }
            } else if (*format == 'c') {
                char c = (char)va_arg(ap, int);
                str[i++] = c;
            } else if (*format == 'p') {
                unsigned long long val = (unsigned long long)(uintptr_t)va_arg(ap, void*);
                char buf[32]; int bi = 0;
                if (val == 0) { buf[bi++] = '0'; }
                else { while (val) { buf[bi++] = "0123456789abcdef"[val & 0xF]; val >>= 4; } }
                for (int s=0, e=bi-1; s<e; s++, e--) { char t=buf[s]; buf[s]=buf[e]; buf[e]=t; }
                if (i<size-1) str[i++] = '0';
                if (i<size-1) str[i++] = 'x';
                for (int p=0; p<bi && i<size-1; p++) str[i++] = buf[p];
            } else if (*format == '%') {
                str[i++] = '%';
            }
            if (*format) format++;
        } else {
            str[i++] = *format++;
        }
    }
    str[i] = 0;
    return i;
}
int snprintf(char* str, size_t size, const char* format, ...) {
    va_list ap; va_start(ap, format);
    int res = vsnprintf(str, size, format, ap);
    va_end(ap); return res;
}
int _vsnprintf(char* str, size_t size, const char* format, va_list ap) {
    return vsnprintf(str, size, format, ap);
}
int sprintf(char* str, const char* format, ...) {
    va_list ap; va_start(ap, format);
    int res = vsnprintf(str, 1024, format, ap);
    va_end(ap); return res;
}
int fprintf(FILE* stream, const char* format, ...) { return 0; }
void exit(int status) { extern void bos_exit(void); bos_exit(); }
int remove(const char* filename) { return -1; }
char* strdup(const char* s) {
    size_t len = strlen(s)+1;
    char* d = malloc(len);
    if(d) memcpy(d, s, len);
    return d;
}
int sscanf(const char* s, const char* format, ...) { return 0; }
void* memmove(void* dest, const void* src, size_t n) { char* d=dest; const char* s=src; if(d<s){ while(n--) *d++=*s++; } else { d+=n; s+=n; while(n--) *--d=*--s; } return dest; }
int rename(const char* old_filename, const char* new_filename) { return -1; }
int fflush(FILE* stream) { return 0; }
int putchar(int c) { return c; }

extern void bos_print(const char* str);
int vfprintf(FILE* stream, const char* format, va_list ap) { 
    char buf[1024];
    vsnprintf(buf, sizeof(buf), format, ap);
    bos_print(buf); 
    return 0; 
}
int system(const char* command) { return -1; }
int errno = 0;
double atof(const char* str) { return 0.0; }
size_t fwrite(const void* ptr, size_t size, size_t nmemb, FILE* stream) { return 0; }
int mkdir(const char *pathname, int mode) { return -1; }

extern void bos_print(const char* str);
int printf(const char* format, ...) { 
    char buf[1024];
    va_list ap;
    va_start(ap, format);
    vsnprintf(buf, sizeof(buf), format, ap);
    va_end(ap);
    bos_print(buf); 
    return 0; 
}
extern int bos_open(const char* path);
extern int bos_close(int fd);
extern int bos_read(int fd, void* buffer, size_t size);
extern int bos_seek(int fd, uint64_t offset, int whence);

size_t fread(void* ptr, size_t size, size_t nmemb, FILE* stream) {
    if (!stream) return 0;
    int fd = (int)(uintptr_t)stream;
    int res = bos_read(fd, ptr, size * nmemb);
    if (res < 0) return 0;
    return res / size;
}

int fclose(FILE* stream) {
    if (!stream) return 0;
    int fd = (int)(uintptr_t)stream;
    return bos_close(fd);
}

FILE* fopen(const char* path, const char* mode) {
    int fd = bos_open(path);
    if (fd < 0) return NULL;
    return (FILE*)(uintptr_t)fd;
}

int fseek(FILE* stream, long offset, int whence) {
    if (!stream) return -1;
    int fd = (int)(uintptr_t)stream;
    int res = bos_seek(fd, offset, whence);
    if (res < 0) return -1;
    return 0;
}

long ftell(FILE* stream) {
    if (!stream) return -1;
    int fd = (int)(uintptr_t)stream;
    return bos_seek(fd, 0, SEEK_CUR);
}
int puts(const char* s) { extern void bos_print(const char* str); bos_print(s); bos_print("\n"); return 0; }
void* malloc(size_t size) { 
    static unsigned char heap[32*1024*1024]; 
    static size_t idx=0; 
    void* p = &heap[idx]; 
    idx += size; 
    return p; 
}
void free(void* ptr) {}
void* calloc(size_t num, size_t size) { void* p=malloc(num*size); if(p) memset(p,0,num*size); return p; }
void* realloc(void* ptr, size_t new_size) { return malloc(new_size); }
char* strstr(const char* haystack, const char* needle) {
    if (!*needle) return (char*)haystack;
    for (; *haystack; haystack++) {
        if (*haystack == *needle) {
            const char* h=haystack, *n=needle;
            while(*h && *n && *h==*n) { h++; n++; }
            if (!*n) return (char*)haystack;
        }
    }
    return NULL;
}
char* strrchr(const char* s, int c) {
    const char* last = NULL;
    do { if(*s==c) last=s; } while(*s++);
    return (char*)last;
}
double fabs(double x) { return x<0 ? -x : x; }
