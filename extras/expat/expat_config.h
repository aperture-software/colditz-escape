// lixpexat conf for PSP
//////////////////////////

#ifndef EXPAT_CONFIG_H
#define EXPAT_CONFIG_H

/* 1234 = LIL_ENDIAN, 4321 = BIGENDIAN */
#define BYTEORDER 1234

/* Define to 1 if you have the `bcopy' function. */
#define HAVE_BCOPY 1

/* Define to 1 if you have the <check.h> header file. */
#undef HAVE_CHECK_H

/* Define to 1 if you have the `memmove' function. */
#undef HAVE_MEMMOVE

/* Define to 1 if you have the <unistd.h> header file. */
#define HAVE_UNISTD_H 1

/* Define to specify how much context to retain around the current parse
   point. */
#define XML_CONTEXT_BYTES 1024

/* Define to make parameter entity parsing functionality available. */
#undef XML_DTD

/* Define to make XML Namespaces functionality available. */
#undef XML_NS

#endif  /* EXPAT_CONFIG_H */
