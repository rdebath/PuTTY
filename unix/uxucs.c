#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <locale.h>
#include <limits.h>
#include <wchar.h>

#include <time.h>
#include <langinfo.h>

#include "putty.h"
#include "charset.h"
#include "terminal.h"
#include "misc.h"

/*
 * Unix Unicode-handling routines.
 */

int mb_to_wc(int codepage, int flags, const char *mbstr, int mblen,
	     wchar_t *wcstr, int wclen)
{
    if (codepage == DEFAULT_CODEPAGE) {
	int n = 0;
	mbstate_t state;

	memset(&state, 0, sizeof state);

	while (mblen > 0) {
	    size_t i = mbrtowc(wcstr+n, mbstr, (size_t)mblen, &state);
	    if (i == (size_t)-1 || i == (size_t)-2)
		break;
	    n++;
	    mbstr += i;
	    mblen -= i;
	}

	return n;
    } else if (codepage == CS_NONE) {
	int n = 0;

	while (mblen > 0) {
	    wcstr[n] = CSET_ACP | (mbstr[0] & 0xFF);
	    n++;
	    mbstr++;
	    mblen--;
	}

	return n;
    } else
	return charset_to_unicode(&mbstr, &mblen, wcstr, wclen, codepage,
				  NULL, NULL, 0);
}

int wc_to_mb(int codepage, int flags, const wchar_t *wcstr, int wclen,
	     char *mbstr, int mblen, const char *defchr, int *defused,
	     struct unicode_data *ucsdata)
{
    /* FIXME: we should remove the defused param completely... */
    if (defused)
	*defused = 0;

    if (codepage == DEFAULT_CODEPAGE) {
	char output[MB_LEN_MAX];
	mbstate_t state;
	int n = 0;

	memset(&state, 0, sizeof state);

	while (wclen > 0) {
	    int i = wcrtomb(output, wcstr[0], &state);
	    if (i == (size_t)-1 || i > n - mblen)
		break;
	    memcpy(mbstr+n, output, i);
	    n += i;
	    wcstr++;
	    wclen--;
	}

	return n;
    } else if (codepage == CS_NONE) {
	int n = 0;
	while (wclen > 0 && n < mblen) {
	    if (*wcstr >= CSET_ACP && *wcstr < CSET_ACP + 0x100)
		mbstr[n++] = (*wcstr & 0xFF);
	    else if (defchr)
		mbstr[n++] = *defchr;
	    wcstr++;
	    wclen--;
	}
	return n;
    } else {
	return charset_from_unicode(&wcstr, &wclen, mbstr, mblen, codepage,
				    NULL, defchr?defchr:NULL, defchr?1:0);
    }
}

/*
 * Return value is TRUE if pterm is to run in direct-to-font mode.
 */
int init_ucs(struct unicode_data *ucsdata, char *linecharset,
	     int utf8_override, int font_charset, int vtmode)
{
    int i, ret = 0;

    /*
     * In the platform-independent parts of the code, font_codepage
     * is used only for system DBCS support - which we don't
     * support at all. So we set this to something which will never
     * be used.
     */
    ucsdata->font_codepage = -1;

    /*
     * If utf8_override is set and the POSIX locale settings
     * dictate a UTF-8 character set, then force defaults to
     * UTF-8.
     */
    ucsdata->utf8_locale = 0;
    if (utf8_override) {
	/* Need to set the locale here because guessing it from the
	 * envionment is nasty; see:
	 * http://www.cl.cam.ac.uk/~mgk25/ucs/langinfo.c
	 */
	setlocale(LC_CTYPE, "");
	if (!strcmp("UTF-8", nl_langinfo(CODESET)))
	    ucsdata->utf8_locale = 1;
    }

    /*
     * The line_codepage 8bit codepage should be decoded from the
     * specification in conf. (This may also be UTF-8)
     */
    ucsdata->line_codepage = decode_codepage(linecharset);

    /*
     * If line_codepage is _still_ CS_NONE, we assume we're using
     * the font's own encoding. This has been passed in to us, so
     * we use that. If it's still CS_NONE after _that_ - i.e. the
     * font we were given had an incomprehensible charset - then we
     * fall back to using the CSET_ACP page.
     */
    if (ucsdata->line_codepage == CS_NONE)
	ucsdata->line_codepage = font_charset;

    if (ucsdata->line_codepage == CS_NONE)
	ret = 1;

    /*
     * Set up unitab_line, by translating each individual character
     * in the line codepage into Unicode.
     */
    for (i = 0; i < 256; i++) {
	char c[1];
        const char *p;
	wchar_t wc[1];
	int len;
	c[0] = i;
	p = c;
	len = 1;
	if (ucsdata->line_codepage == CS_NONE)
	    ucsdata->unitab_line[i] = CSET_ACP | i;
	else if (1 == charset_to_unicode(&p, &len, wc, 1,
					 ucsdata->line_codepage,
					 NULL, L"", 0))
	    ucsdata->unitab_line[i] = wc[0];
	else if (ucsdata->line_codepage == CS_UTF8)
	    ucsdata->unitab_line[i] = i; /* Use ISO-8859-1 */
	else
	    ucsdata->unitab_line[i] = 0xFFFD;
    }

    /*
     * Set up unitab_xterm. This is the same as unitab_line except
     * in the line-drawing regions, where it follows the Unicode
     * encoding.
     * 
     * (Note that the strange X encoding of line-drawing characters
     * in the bottom 32 glyphs of ISO8859-1 fonts is taken care of
     * by the font encoding, which will spot such a font and act as
     * if it were in a variant encoding of ISO8859-1.)
     */
    for (i = 0; i < 256; i++) {
	static const wchar_t unitab_xterm_std[32] = {
	    0x2666, 0x2592, 0x2409, 0x240c, 0x240d, 0x240a, 0x00b0, 0x00b1,
	    0x2424, 0x240b, 0x2518, 0x2510, 0x250c, 0x2514, 0x253c, 0x23ba,
	    0x23bb, 0x2500, 0x23bc, 0x23bd, 0x251c, 0x2524, 0x2534, 0x252c,
	    0x2502, 0x2264, 0x2265, 0x03c0, 0x2260, 0x00a3, 0x00b7, 0x0020
	};
	static const wchar_t unitab_xterm_poorman[32] =
	    L"*#****o~**+++++-----++++|****L. ";

	const wchar_t *ptr;

	if (vtmode == VT_POORMAN)
	    ptr = unitab_xterm_poorman;
	else
	    ptr = unitab_xterm_std;

	if (i >= 0x5F && i < 0x7F)
	    ucsdata->unitab_xterm[i] = ptr[i & 0x1F];
	else
	    ucsdata->unitab_xterm[i] = ucsdata->unitab_line[i];
    }

    /*
     * Set up unitab_scoacs. The SCO Alternate Character Set is
     * simply CP437.
     *
     * Except ... for this codeset characters 1..31 and 127 are
     * replaced by glyph characters common to all MS OEM codepages.
     */
    for (i = 0; i < 256; i++) {
	static int oemcp_glyph_overlay[] = {
	    0x2302, 0x263a, 0x263b, 0x2665, 0x2666, 0x2663, 0x2660, 0x2022,
	    0x25d8, 0x25cb, 0x25d9, 0x2642, 0x2640, 0x266a, 0x266b, 0x263c,
	    0x25b6, 0x25c0, 0x2195, 0x203c, 0x00b6, 0x00a7, 0x25ac, 0x21a8,
	    0x2191, 0x2193, 0x2192, 0x2190, 0x221f, 0x2194, 0x25b2, 0x25bc
	};
	char c[1];
        const char *p;
	wchar_t wc[1];
	int len;
	c[0] = i;
	p = c;
	len = 1;
	if (i >= 1 && i < 32)
	    ucsdata->unitab_scoacs[i] = oemcp_glyph_overlay[i];
	else if (i == 127)
	    ucsdata->unitab_scoacs[i] = oemcp_glyph_overlay[0];
	else
	if (1 == charset_to_unicode(&p, &len, wc, 1, CS_CP437, NULL, L"", 0))
	    ucsdata->unitab_scoacs[i] = wc[0];
	else
	    ucsdata->unitab_scoacs[i] = 0xFFFD;
    }

    /*
     * Find the control characters in the line codepage. For
     * direct-to-font mode using the CSET_ACP hack, we assume 00-1F and
     * 7F are controls, but allow 80-9F through. (It's as good a
     * guess as anything; and my bet is that half the weird fonts
     * used in this way will be IBM or MS code pages anyway.)
     */
    for (i = 0; i < 256; i++) {
	int lineval = ucsdata->unitab_line[i];
	if (lineval < ' ' || (lineval >= 0x7F && lineval < 0xA0) ||
	    (lineval >= CSET_ACP && lineval < CSET_ACP + 0x20) ||
	    (lineval == CSET_ACP + 0x7F))
	    ucsdata->unitab_ctrl[i] = i;
	else
	    ucsdata->unitab_ctrl[i] = 0xFF;
    }

    return ret;
}

const char *cp_name(int codepage)
{
    if (codepage == CS_NONE)
	return "Use font encoding";
    return charset_to_localenc(codepage);
}

const char *cp_enumerate(int index)
{
    int charset;
    charset = charset_localenc_nth(index);
    if (charset == CS_NONE) {
        /* "Use font encoding" comes after all the named charsets */
        if (charset_localenc_nth(index-1) != CS_NONE)
            return "Use font encoding";
	return NULL;
    }
    return charset_to_localenc(charset);
}

int decode_codepage(char *cp_name)
{
    if (!cp_name || !*cp_name)
	return CS_UTF8;
    return charset_from_localenc(cp_name);
}
