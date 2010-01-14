dnl vim:se ts=2 sw=2 et:

PHP_ARG_ENABLE(textcat, whether to enable textcat functions,
[  --enable-textcat         Enable textcat support])

if test "$PHP_TEXTCAT" != "no"; then

  PHP_SUBST(TEXTCAT_SHARED_LIBADD)
  AC_DEFINE(HAVE_TEXTCAT, 1, [ ])

  PHP_NEW_EXTENSION(textcat, php_textcat.c textcat/ngrams.c textcat/memory.c textcat/handlers.c textcat/textcat.c, $ext_shared)
fi

