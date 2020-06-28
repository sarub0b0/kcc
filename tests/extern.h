

#ifndef __FOO_H
#define __FOO_H

int extern_a = 11;

#ifdef __cplusplus
#define __BEGIN_DECLS extern "C" {
#define __END_DECLS }
#else
#define __BEGIN_DECLS
#define __END_DECLS
#endif

// // extern "C" {

__BEGIN_DECLS
int foo();
__END_DECLS
// }

#endif

