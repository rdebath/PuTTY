/*
 * This is an implementation of wcwidth() and wcswidth() (defined in
 * IEEE Std 1002.1-2001) for Unicode.
 *
 * http://www.opengroup.org/onlinepubs/007904975/functions/wcwidth.html
 * http://www.opengroup.org/onlinepubs/007904975/functions/wcswidth.html
 *
 * In fixed-width output devices, Latin characters all occupy a single
 * "cell" position of equal width, whereas ideographic CJK characters
 * occupy two such cells. Interoperability between terminal-line
 * applications and (teletype-style) character terminals using the
 * UTF-8 encoding requires agreement on which character should advance
 * the cursor by how many cell positions. No established formal
 * standards exist at present on which Unicode character shall occupy
 * how many cell positions on character terminals. These routines are
 * a first attempt of defining such behavior based on simple rules
 * applied to data provided by the Unicode Consortium.
 *
 * For some graphical characters, the Unicode standard explicitly
 * defines a character-cell width via the definition of the East Asian
 * FullWidth (F), Wide (W), Half-width (H), and Narrow (Na) classes.
 * In all these cases, there is no ambiguity about which width a
 * terminal shall use. For characters in the East Asian Ambiguous (A)
 * class, the width choice depends purely on a preference of backward
 * compatibility with either historic CJK or Western practice.
 * Choosing single-width for these characters is easy to justify as
 * the appropriate long-term solution, as the CJK practice of
 * displaying these characters as double-width comes from historic
 * implementation simplicity (8-bit encoded characters were displayed
 * single-width and 16-bit ones double-width, even for Greek,
 * Cyrillic, etc.) and not any typographic considerations.
 *
 * Much less clear is the choice of width for the Not East Asian
 * (Neutral) class. Existing practice does not dictate a width for any
 * of these characters. It would nevertheless make sense
 * typographically to allocate two character cells to characters such
 * as for instance EM SPACE or VOLUME INTEGRAL, which cannot be
 * represented adequately with a single-width glyph. The following
 * routines at present merely assign a single-cell width to all
 * neutral characters, in the interest of simplicity. This is not
 * entirely satisfactory and should be reconsidered before
 * establishing a formal standard in this area. At the moment, the
 * decision which Not East Asian (Neutral) characters should be
 * represented by double-width glyphs cannot yet be answered by
 * applying a simple rule from the Unicode database content. Setting
 * up a proper standard for the behavior of UTF-8 character terminals
 * will require a careful analysis not only of each Unicode character,
 * but also of each presentation form, something the author of these
 * routines has avoided to do so far.
 *
 * http://www.unicode.org/unicode/reports/tr11/
 *
 * Markus Kuhn -- 2007-05-26 (Unicode 5.0)
 *
 * Permission to use, copy, modify, and distribute this software
 * for any purpose and without fee is hereby granted. The author
 * disclaims all warranties with regard to this software.
 *
 * Latest version: http://www.cl.cam.ac.uk/~mgk25/ucs/wcwidth.c
 */

#include <wchar.h>

#include "putty.h" /* for prototypes */

struct interval {
  unsigned int first;
  unsigned int last;
};

/* auxiliary function for binary search in interval table */
static int bisearch(unsigned int ucs, const struct interval *table, int max) {
  int min = 0;
  int mid;

  if (ucs < table[0].first || ucs > table[max].last)
    return 0;
  while (max >= min) {
    mid = (min + max) / 2;
    if (ucs > table[mid].last)
      min = mid + 1;
    else if (ucs < table[mid].first)
      max = mid - 1;
    else
      return 1;
  }

  return 0;
}


/* The following two functions define the column width of an ISO 10646
 * character as follows:
 *
 *    - The null character (U+0000) has a column width of 0.
 *
 *    - Other C0/C1 control characters and DEL will lead to a return
 *      value of -1.
 *
 *    - Non-spacing and enclosing combining characters (general
 *      category code Mn or Me in the Unicode database) have a
 *      column width of 0.
 *
 *    - SOFT HYPHEN (U+00AD) has a column width of 1.
 *
 *    - Other format characters (general category code Cf in the Unicode
 *      database) and ZERO WIDTH SPACE (U+200B) have a column width of 0.
 *
 *    - Hangul Jamo medial vowels and final consonants (U+1160-U+11FF)
 *      have a column width of 0.
 *
 *    - Spacing characters in the East Asian Wide (W) or East Asian
 *      Full-width (F) category as defined in Unicode Technical
 *      Report #11 have a column width of 2.
 *
 *    - All remaining characters (including all printable
 *      ISO 8859-1 and WGL4 characters, Unicode control characters,
 *      etc.) have a column width of 1.
 *
 * This implementation assumes that wchar_t characters are encoded
 * in ISO 10646.
 */

int mk_wcwidth(unsigned int ucs)
{
  /* sorted list of non-overlapping intervals of non-spacing characters */
  /* All Mn, Me and Cf characters from version 10.0.0 of
     http://www.unicode.org/Public/UNIDATA/extracted/DerivedGeneralCategory.txt
   */

  static const struct interval combining[] = {
    {0x00ad, 0x00ad},	/* SOFT HYPHEN */
    {0x0300, 0x036f},	/* COMBINING GRAVE ACCENT to COMBINING LATIN SMALL LETTER X */
    {0x0483, 0x0489},	/* COMBINING CYRILLIC TITLO to COMBINING CYRILLIC MILLIONS SIGN */
    {0x0591, 0x05bd},	/* HEBREW ACCENT ETNAHTA to HEBREW POINT METEG */
    {0x05bf, 0x05bf},	/* HEBREW POINT RAFE */
    {0x05c1, 0x05c2},	/* HEBREW POINT SHIN DOT to HEBREW POINT SIN DOT */
    {0x05c4, 0x05c5},	/* HEBREW MARK UPPER DOT to HEBREW MARK LOWER DOT */
    {0x05c7, 0x05c7},	/* HEBREW POINT QAMATS QATAN */
    {0x0600, 0x0605},	/* ARABIC NUMBER SIGN to ARABIC NUMBER MARK ABOVE */
    {0x0610, 0x061a},	/* ARABIC SIGN SALLALLAHOU ALAYHE WASSALLAM to ARABIC SMALL KASRA */
    {0x061c, 0x061c},	/* ARABIC LETTER MARK */
    {0x064b, 0x065f},	/* ARABIC FATHATAN to ARABIC WAVY HAMZA BELOW */
    {0x0670, 0x0670},	/* ARABIC LETTER SUPERSCRIPT ALEF */
    {0x06d6, 0x06dd},	/* ARABIC SMALL HIGH LIGATURE SAD WITH LAM WITH ALEF MAKSURA to ARABIC END OF AYAH */
    {0x06df, 0x06e4},	/* ARABIC SMALL HIGH ROUNDED ZERO to ARABIC SMALL HIGH MADDA */
    {0x06e7, 0x06e8},	/* ARABIC SMALL HIGH YEH to ARABIC SMALL HIGH NOON */
    {0x06ea, 0x06ed},	/* ARABIC EMPTY CENTRE LOW STOP to ARABIC SMALL LOW MEEM */
    {0x070f, 0x070f},	/* SYRIAC ABBREVIATION MARK */
    {0x0711, 0x0711},	/* SYRIAC LETTER SUPERSCRIPT ALAPH */
    {0x0730, 0x074a},	/* SYRIAC PTHAHA ABOVE to SYRIAC BARREKH */
    {0x07a6, 0x07b0},	/* THAANA ABAFILI to THAANA SUKUN */
    {0x07eb, 0x07f3},	/* NKO COMBINING SHORT HIGH TONE to NKO COMBINING DOUBLE DOT ABOVE */
    {0x0816, 0x0819},	/* SAMARITAN MARK IN to SAMARITAN MARK DAGESH */
    {0x081b, 0x0823},	/* SAMARITAN MARK EPENTHETIC YUT to SAMARITAN VOWEL SIGN A */
    {0x0825, 0x0827},	/* SAMARITAN VOWEL SIGN SHORT A to SAMARITAN VOWEL SIGN U */
    {0x0829, 0x082d},	/* SAMARITAN VOWEL SIGN LONG I to SAMARITAN MARK NEQUDAA */
    {0x0859, 0x085b},	/* MANDAIC AFFRICATION MARK to MANDAIC GEMINATION MARK */
    {0x08d4, 0x0902},	/* ARABIC SMALL HIGH WORD AR-RUB to DEVANAGARI SIGN ANUSVARA */
    {0x093a, 0x093a},	/* DEVANAGARI VOWEL SIGN OE */
    {0x093c, 0x093c},	/* DEVANAGARI SIGN NUKTA */
    {0x0941, 0x0948},	/* DEVANAGARI VOWEL SIGN U to DEVANAGARI VOWEL SIGN AI */
    {0x094d, 0x094d},	/* DEVANAGARI SIGN VIRAMA */
    {0x0951, 0x0957},	/* DEVANAGARI STRESS SIGN UDATTA to DEVANAGARI VOWEL SIGN UUE */
    {0x0962, 0x0963},	/* DEVANAGARI VOWEL SIGN VOCALIC L to DEVANAGARI VOWEL SIGN VOCALIC LL */
    {0x0981, 0x0981},	/* BENGALI SIGN CANDRABINDU */
    {0x09bc, 0x09bc},	/* BENGALI SIGN NUKTA */
    {0x09c1, 0x09c4},	/* BENGALI VOWEL SIGN U to BENGALI VOWEL SIGN VOCALIC RR */
    {0x09cd, 0x09cd},	/* BENGALI SIGN VIRAMA */
    {0x09e2, 0x09e3},	/* BENGALI VOWEL SIGN VOCALIC L to BENGALI VOWEL SIGN VOCALIC LL */
    {0x0a01, 0x0a02},	/* GURMUKHI SIGN ADAK BINDI to GURMUKHI SIGN BINDI */
    {0x0a3c, 0x0a3c},	/* GURMUKHI SIGN NUKTA */
    {0x0a41, 0x0a42},	/* GURMUKHI VOWEL SIGN U to GURMUKHI VOWEL SIGN UU */
    {0x0a47, 0x0a48},	/* GURMUKHI VOWEL SIGN EE to GURMUKHI VOWEL SIGN AI */
    {0x0a4b, 0x0a4d},	/* GURMUKHI VOWEL SIGN OO to GURMUKHI SIGN VIRAMA */
    {0x0a51, 0x0a51},	/* GURMUKHI SIGN UDAAT */
    {0x0a70, 0x0a71},	/* GURMUKHI TIPPI to GURMUKHI ADDAK */
    {0x0a75, 0x0a75},	/* GURMUKHI SIGN YAKASH */
    {0x0a81, 0x0a82},	/* GUJARATI SIGN CANDRABINDU to GUJARATI SIGN ANUSVARA */
    {0x0abc, 0x0abc},	/* GUJARATI SIGN NUKTA */
    {0x0ac1, 0x0ac5},	/* GUJARATI VOWEL SIGN U to GUJARATI VOWEL SIGN CANDRA E */
    {0x0ac7, 0x0ac8},	/* GUJARATI VOWEL SIGN E to GUJARATI VOWEL SIGN AI */
    {0x0acd, 0x0acd},	/* GUJARATI SIGN VIRAMA */
    {0x0ae2, 0x0ae3},	/* GUJARATI VOWEL SIGN VOCALIC L to GUJARATI VOWEL SIGN VOCALIC LL */
    {0x0afa, 0x0aff},	/* GUJARATI SIGN SUKUN to GUJARATI SIGN TWO-CIRCLE NUKTA ABOVE */
    {0x0b01, 0x0b01},	/* ORIYA SIGN CANDRABINDU */
    {0x0b3c, 0x0b3c},	/* ORIYA SIGN NUKTA */
    {0x0b3f, 0x0b3f},	/* ORIYA VOWEL SIGN I */
    {0x0b41, 0x0b44},	/* ORIYA VOWEL SIGN U to ORIYA VOWEL SIGN VOCALIC RR */
    {0x0b4d, 0x0b4d},	/* ORIYA SIGN VIRAMA */
    {0x0b56, 0x0b56},	/* ORIYA AI LENGTH MARK */
    {0x0b62, 0x0b63},	/* ORIYA VOWEL SIGN VOCALIC L to ORIYA VOWEL SIGN VOCALIC LL */
    {0x0b82, 0x0b82},	/* TAMIL SIGN ANUSVARA */
    {0x0bc0, 0x0bc0},	/* TAMIL VOWEL SIGN II */
    {0x0bcd, 0x0bcd},	/* TAMIL SIGN VIRAMA */
    {0x0c00, 0x0c00},	/* TELUGU SIGN COMBINING CANDRABINDU ABOVE */
    {0x0c3e, 0x0c40},	/* TELUGU VOWEL SIGN AA to TELUGU VOWEL SIGN II */
    {0x0c46, 0x0c48},	/* TELUGU VOWEL SIGN E to TELUGU VOWEL SIGN AI */
    {0x0c4a, 0x0c4d},	/* TELUGU VOWEL SIGN O to TELUGU SIGN VIRAMA */
    {0x0c55, 0x0c56},	/* TELUGU LENGTH MARK to TELUGU AI LENGTH MARK */
    {0x0c62, 0x0c63},	/* TELUGU VOWEL SIGN VOCALIC L to TELUGU VOWEL SIGN VOCALIC LL */
    {0x0c81, 0x0c81},	/* KANNADA SIGN CANDRABINDU */
    {0x0cbc, 0x0cbc},	/* KANNADA SIGN NUKTA */
    {0x0cbf, 0x0cbf},	/* KANNADA VOWEL SIGN I */
    {0x0cc6, 0x0cc6},	/* KANNADA VOWEL SIGN E */
    {0x0ccc, 0x0ccd},	/* KANNADA VOWEL SIGN AU to KANNADA SIGN VIRAMA */
    {0x0ce2, 0x0ce3},	/* KANNADA VOWEL SIGN VOCALIC L to KANNADA VOWEL SIGN VOCALIC LL */
    {0x0d00, 0x0d01},	/* MALAYALAM SIGN COMBINING ANUSVARA ABOVE to MALAYALAM SIGN CANDRABINDU */
    {0x0d3b, 0x0d3c},	/* MALAYALAM SIGN VERTICAL BAR VIRAMA to MALAYALAM SIGN CIRCULAR VIRAMA */
    {0x0d41, 0x0d44},	/* MALAYALAM VOWEL SIGN U to MALAYALAM VOWEL SIGN VOCALIC RR */
    {0x0d4d, 0x0d4d},	/* MALAYALAM SIGN VIRAMA */
    {0x0d62, 0x0d63},	/* MALAYALAM VOWEL SIGN VOCALIC L to MALAYALAM VOWEL SIGN VOCALIC LL */
    {0x0dca, 0x0dca},	/* SINHALA SIGN AL-LAKUNA */
    {0x0dd2, 0x0dd4},	/* SINHALA VOWEL SIGN KETTI IS-PILLA to SINHALA VOWEL SIGN KETTI PAA-PILLA */
    {0x0dd6, 0x0dd6},	/* SINHALA VOWEL SIGN DIGA PAA-PILLA */
    {0x0e31, 0x0e31},	/* THAI CHARACTER MAI HAN-AKAT */
    {0x0e34, 0x0e3a},	/* THAI CHARACTER SARA I to THAI CHARACTER PHINTHU */
    {0x0e47, 0x0e4e},	/* THAI CHARACTER MAITAIKHU to THAI CHARACTER YAMAKKAN */
    {0x0eb1, 0x0eb1},	/* LAO VOWEL SIGN MAI KAN */
    {0x0eb4, 0x0eb9},	/* LAO VOWEL SIGN I to LAO VOWEL SIGN UU */
    {0x0ebb, 0x0ebc},	/* LAO VOWEL SIGN MAI KON to LAO SEMIVOWEL SIGN LO */
    {0x0ec8, 0x0ecd},	/* LAO TONE MAI EK to LAO NIGGAHITA */
    {0x0f18, 0x0f19},	/* TIBETAN ASTROLOGICAL SIGN -KHYUD PA to TIBETAN ASTROLOGICAL SIGN SDONG TSHUGS */
    {0x0f35, 0x0f35},	/* TIBETAN MARK NGAS BZUNG NYI ZLA */
    {0x0f37, 0x0f37},	/* TIBETAN MARK NGAS BZUNG SGOR RTAGS */
    {0x0f39, 0x0f39},	/* TIBETAN MARK TSA -PHRU */
    {0x0f71, 0x0f7e},	/* TIBETAN VOWEL SIGN AA to TIBETAN SIGN RJES SU NGA RO */
    {0x0f80, 0x0f84},	/* TIBETAN VOWEL SIGN REVERSED I to TIBETAN MARK HALANTA */
    {0x0f86, 0x0f87},	/* TIBETAN SIGN LCI RTAGS to TIBETAN SIGN YANG RTAGS */
    {0x0f8d, 0x0f97},	/* TIBETAN SUBJOINED SIGN LCE TSA CAN to TIBETAN SUBJOINED LETTER JA */
    {0x0f99, 0x0fbc},	/* TIBETAN SUBJOINED LETTER NYA to TIBETAN SUBJOINED LETTER FIXED-FORM RA */
    {0x0fc6, 0x0fc6},	/* TIBETAN SYMBOL PADMA GDAN */
    {0x102d, 0x1030},	/* MYANMAR VOWEL SIGN I to MYANMAR VOWEL SIGN UU */
    {0x1032, 0x1037},	/* MYANMAR VOWEL SIGN AI to MYANMAR SIGN DOT BELOW */
    {0x1039, 0x103a},	/* MYANMAR SIGN VIRAMA to MYANMAR SIGN ASAT */
    {0x103d, 0x103e},	/* MYANMAR CONSONANT SIGN MEDIAL WA to MYANMAR CONSONANT SIGN MEDIAL HA */
    {0x1058, 0x1059},	/* MYANMAR VOWEL SIGN VOCALIC L to MYANMAR VOWEL SIGN VOCALIC LL */
    {0x105e, 0x1060},	/* MYANMAR CONSONANT SIGN MON MEDIAL NA to MYANMAR CONSONANT SIGN MON MEDIAL LA */
    {0x1071, 0x1074},	/* MYANMAR VOWEL SIGN GEBA KAREN I to MYANMAR VOWEL SIGN KAYAH EE */
    {0x1082, 0x1082},	/* MYANMAR CONSONANT SIGN SHAN MEDIAL WA */
    {0x1085, 0x1086},	/* MYANMAR VOWEL SIGN SHAN E ABOVE to MYANMAR VOWEL SIGN SHAN FINAL Y */
    {0x108d, 0x108d},	/* MYANMAR SIGN SHAN COUNCIL EMPHATIC TONE */
    {0x109d, 0x109d},	/* MYANMAR VOWEL SIGN AITON AI */
    {0x135d, 0x135f},	/* ETHIOPIC COMBINING GEMINATION AND VOWEL LENGTH MARK to ETHIOPIC COMBINING GEMINATION MARK */
    {0x1712, 0x1714},	/* TAGALOG VOWEL SIGN I to TAGALOG SIGN VIRAMA */
    {0x1732, 0x1734},	/* HANUNOO VOWEL SIGN I to HANUNOO SIGN PAMUDPOD */
    {0x1752, 0x1753},	/* BUHID VOWEL SIGN I to BUHID VOWEL SIGN U */
    {0x1772, 0x1773},	/* TAGBANWA VOWEL SIGN I to TAGBANWA VOWEL SIGN U */
    {0x17b4, 0x17b5},	/* KHMER VOWEL INHERENT AQ to KHMER VOWEL INHERENT AA */
    {0x17b7, 0x17bd},	/* KHMER VOWEL SIGN I to KHMER VOWEL SIGN UA */
    {0x17c6, 0x17c6},	/* KHMER SIGN NIKAHIT */
    {0x17c9, 0x17d3},	/* KHMER SIGN MUUSIKATOAN to KHMER SIGN BATHAMASAT */
    {0x17dd, 0x17dd},	/* KHMER SIGN ATTHACAN */
    {0x180b, 0x180e},	/* MONGOLIAN FREE VARIATION SELECTOR ONE to MONGOLIAN VOWEL SEPARATOR */
    {0x1885, 0x1886},	/* MONGOLIAN LETTER ALI GALI BALUDA to MONGOLIAN LETTER ALI GALI THREE BALUDA */
    {0x18a9, 0x18a9},	/* MONGOLIAN LETTER ALI GALI DAGALGA */
    {0x1920, 0x1922},	/* LIMBU VOWEL SIGN A to LIMBU VOWEL SIGN U */
    {0x1927, 0x1928},	/* LIMBU VOWEL SIGN E to LIMBU VOWEL SIGN O */
    {0x1932, 0x1932},	/* LIMBU SMALL LETTER ANUSVARA */
    {0x1939, 0x193b},	/* LIMBU SIGN MUKPHRENG to LIMBU SIGN SA-I */
    {0x1a17, 0x1a18},	/* BUGINESE VOWEL SIGN I to BUGINESE VOWEL SIGN U */
    {0x1a1b, 0x1a1b},	/* BUGINESE VOWEL SIGN AE */
    {0x1a56, 0x1a56},	/* TAI THAM CONSONANT SIGN MEDIAL LA */
    {0x1a58, 0x1a5e},	/* TAI THAM SIGN MAI KANG LAI to TAI THAM CONSONANT SIGN SA */
    {0x1a60, 0x1a60},	/* TAI THAM SIGN SAKOT */
    {0x1a62, 0x1a62},	/* TAI THAM VOWEL SIGN MAI SAT */
    {0x1a65, 0x1a6c},	/* TAI THAM VOWEL SIGN I to TAI THAM VOWEL SIGN OA BELOW */
    {0x1a73, 0x1a7c},	/* TAI THAM VOWEL SIGN OA ABOVE to TAI THAM SIGN KHUEN-LUE KARAN */
    {0x1a7f, 0x1a7f},	/* TAI THAM COMBINING CRYPTOGRAMMIC DOT */
    {0x1ab0, 0x1abe},	/* COMBINING DOUBLED CIRCUMFLEX ACCENT to COMBINING PARENTHESES OVERLAY */
    {0x1b00, 0x1b03},	/* BALINESE SIGN ULU RICEM to BALINESE SIGN SURANG */
    {0x1b34, 0x1b34},	/* BALINESE SIGN REREKAN */
    {0x1b36, 0x1b3a},	/* BALINESE VOWEL SIGN ULU to BALINESE VOWEL SIGN RA REPA */
    {0x1b3c, 0x1b3c},	/* BALINESE VOWEL SIGN LA LENGA */
    {0x1b42, 0x1b42},	/* BALINESE VOWEL SIGN PEPET */
    {0x1b6b, 0x1b73},	/* BALINESE MUSICAL SYMBOL COMBINING TEGEH to BALINESE MUSICAL SYMBOL COMBINING GONG */
    {0x1b80, 0x1b81},	/* SUNDANESE SIGN PANYECEK to SUNDANESE SIGN PANGLAYAR */
    {0x1ba2, 0x1ba5},	/* SUNDANESE CONSONANT SIGN PANYAKRA to SUNDANESE VOWEL SIGN PANYUKU */
    {0x1ba8, 0x1ba9},	/* SUNDANESE VOWEL SIGN PAMEPET to SUNDANESE VOWEL SIGN PANEULEUNG */
    {0x1bab, 0x1bad},	/* SUNDANESE SIGN VIRAMA to SUNDANESE CONSONANT SIGN PASANGAN WA */
    {0x1be6, 0x1be6},	/* BATAK SIGN TOMPI */
    {0x1be8, 0x1be9},	/* BATAK VOWEL SIGN PAKPAK E to BATAK VOWEL SIGN EE */
    {0x1bed, 0x1bed},	/* BATAK VOWEL SIGN KARO O */
    {0x1bef, 0x1bf1},	/* BATAK VOWEL SIGN U FOR SIMALUNGUN SA to BATAK CONSONANT SIGN H */
    {0x1c2c, 0x1c33},	/* LEPCHA VOWEL SIGN E to LEPCHA CONSONANT SIGN T */
    {0x1c36, 0x1c37},	/* LEPCHA SIGN RAN to LEPCHA SIGN NUKTA */
    {0x1cd0, 0x1cd2},	/* VEDIC TONE KARSHANA to VEDIC TONE PRENKHA */
    {0x1cd4, 0x1ce0},	/* VEDIC SIGN YAJURVEDIC MIDLINE SVARITA to VEDIC TONE RIGVEDIC KASHMIRI INDEPENDENT SVARITA */
    {0x1ce2, 0x1ce8},	/* VEDIC SIGN VISARGA SVARITA to VEDIC SIGN VISARGA ANUDATTA WITH TAIL */
    {0x1ced, 0x1ced},	/* VEDIC SIGN TIRYAK */
    {0x1cf4, 0x1cf4},	/* VEDIC TONE CANDRA ABOVE */
    {0x1cf8, 0x1cf9},	/* VEDIC TONE RING ABOVE to VEDIC TONE DOUBLE RING ABOVE */
    {0x1dc0, 0x1df9},	/* COMBINING DOTTED GRAVE ACCENT to COMBINING WIDE INVERTED BRIDGE BELOW */
    {0x1dfb, 0x1dff},	/* COMBINING DELETION MARK to COMBINING RIGHT ARROWHEAD AND DOWN ARROWHEAD BELOW */
    {0x200b, 0x200f},	/* ZERO WIDTH SPACE to RIGHT-TO-LEFT MARK */
    {0x202a, 0x202e},	/* LEFT-TO-RIGHT EMBEDDING to RIGHT-TO-LEFT OVERRIDE */
    {0x2060, 0x2064},	/* WORD JOINER to INVISIBLE PLUS */
    {0x2066, 0x206f},	/* LEFT-TO-RIGHT ISOLATE to NOMINAL DIGIT SHAPES */
    {0x20d0, 0x20f0},	/* COMBINING LEFT HARPOON ABOVE to COMBINING ASTERISK ABOVE */
    {0x2cef, 0x2cf1},	/* COPTIC COMBINING NI ABOVE to COPTIC COMBINING SPIRITUS LENIS */
    {0x2d7f, 0x2d7f},	/* TIFINAGH CONSONANT JOINER */
    {0x2de0, 0x2dff},	/* COMBINING CYRILLIC LETTER BE to COMBINING CYRILLIC LETTER IOTIFIED BIG YUS */
    {0x302a, 0x302d},	/* IDEOGRAPHIC LEVEL TONE MARK to IDEOGRAPHIC ENTERING TONE MARK */
    {0x3099, 0x309a},	/* COMBINING KATAKANA-HIRAGANA VOICED SOUND MARK to COMBINING KATAKANA-HIRAGANA SEMI-VOICED SOUND MARK */
    {0xa66f, 0xa672},	/* COMBINING CYRILLIC VZMET to COMBINING CYRILLIC THOUSAND MILLIONS SIGN */
    {0xa674, 0xa67d},	/* COMBINING CYRILLIC LETTER UKRAINIAN IE to COMBINING CYRILLIC PAYEROK */
    {0xa69e, 0xa69f},	/* COMBINING CYRILLIC LETTER EF to COMBINING CYRILLIC LETTER IOTIFIED E */
    {0xa6f0, 0xa6f1},	/* BAMUM COMBINING MARK KOQNDON to BAMUM COMBINING MARK TUKWENTIS */
    {0xa802, 0xa802},	/* SYLOTI NAGRI SIGN DVISVARA */
    {0xa806, 0xa806},	/* SYLOTI NAGRI SIGN HASANTA */
    {0xa80b, 0xa80b},	/* SYLOTI NAGRI SIGN ANUSVARA */
    {0xa825, 0xa826},	/* SYLOTI NAGRI VOWEL SIGN U to SYLOTI NAGRI VOWEL SIGN E */
    {0xa8c4, 0xa8c5},	/* SAURASHTRA SIGN VIRAMA to SAURASHTRA SIGN CANDRABINDU */
    {0xa8e0, 0xa8f1},	/* COMBINING DEVANAGARI DIGIT ZERO to COMBINING DEVANAGARI SIGN AVAGRAHA */
    {0xa926, 0xa92d},	/* KAYAH LI VOWEL UE to KAYAH LI TONE CALYA PLOPHU */
    {0xa947, 0xa951},	/* REJANG VOWEL SIGN I to REJANG CONSONANT SIGN R */
    {0xa980, 0xa982},	/* JAVANESE SIGN PANYANGGA to JAVANESE SIGN LAYAR */
    {0xa9b3, 0xa9b3},	/* JAVANESE SIGN CECAK TELU */
    {0xa9b6, 0xa9b9},	/* JAVANESE VOWEL SIGN WULU to JAVANESE VOWEL SIGN SUKU MENDUT */
    {0xa9bc, 0xa9bc},	/* JAVANESE VOWEL SIGN PEPET */
    {0xa9e5, 0xa9e5},	/* MYANMAR SIGN SHAN SAW */
    {0xaa29, 0xaa2e},	/* CHAM VOWEL SIGN AA to CHAM VOWEL SIGN OE */
    {0xaa31, 0xaa32},	/* CHAM VOWEL SIGN AU to CHAM VOWEL SIGN UE */
    {0xaa35, 0xaa36},	/* CHAM CONSONANT SIGN LA to CHAM CONSONANT SIGN WA */
    {0xaa43, 0xaa43},	/* CHAM CONSONANT SIGN FINAL NG */
    {0xaa4c, 0xaa4c},	/* CHAM CONSONANT SIGN FINAL M */
    {0xaa7c, 0xaa7c},	/* MYANMAR SIGN TAI LAING TONE-2 */
    {0xaab0, 0xaab0},	/* TAI VIET MAI KANG */
    {0xaab2, 0xaab4},	/* TAI VIET VOWEL I to TAI VIET VOWEL U */
    {0xaab7, 0xaab8},	/* TAI VIET MAI KHIT to TAI VIET VOWEL IA */
    {0xaabe, 0xaabf},	/* TAI VIET VOWEL AM to TAI VIET TONE MAI EK */
    {0xaac1, 0xaac1},	/* TAI VIET TONE MAI THO */
    {0xaaec, 0xaaed},	/* MEETEI MAYEK VOWEL SIGN UU to MEETEI MAYEK VOWEL SIGN AAI */
    {0xaaf6, 0xaaf6},	/* MEETEI MAYEK VIRAMA */
    {0xabe5, 0xabe5},	/* MEETEI MAYEK VOWEL SIGN ANAP */
    {0xabe8, 0xabe8},	/* MEETEI MAYEK VOWEL SIGN UNAP */
    {0xabed, 0xabed},	/* MEETEI MAYEK APUN IYEK */
    {0xfb1e, 0xfb1e},	/* HEBREW POINT JUDEO-SPANISH VARIKA */
    {0xfe00, 0xfe0f},	/* VARIATION SELECTOR-1 to VARIATION SELECTOR-16 */
    {0xfe20, 0xfe2f},	/* COMBINING LIGATURE LEFT HALF to COMBINING CYRILLIC TITLO RIGHT HALF */
    {0xfeff, 0xfeff},	/* ZERO WIDTH NO-BREAK SPACE */
    {0xfff9, 0xfffb},	/* INTERLINEAR ANNOTATION ANCHOR to INTERLINEAR ANNOTATION TERMINATOR */
    {0x101fd, 0x101fd},	/* PHAISTOS DISC SIGN COMBINING OBLIQUE STROKE */
    {0x102e0, 0x102e0},	/* COPTIC EPACT THOUSANDS MARK */
    {0x10376, 0x1037a},	/* COMBINING OLD PERMIC LETTER AN to COMBINING OLD PERMIC LETTER SII */
    {0x10a01, 0x10a03},	/* KHAROSHTHI VOWEL SIGN I to KHAROSHTHI VOWEL SIGN VOCALIC R */
    {0x10a05, 0x10a06},	/* KHAROSHTHI VOWEL SIGN E to KHAROSHTHI VOWEL SIGN O */
    {0x10a0c, 0x10a0f},	/* KHAROSHTHI VOWEL LENGTH MARK to KHAROSHTHI SIGN VISARGA */
    {0x10a38, 0x10a3a},	/* KHAROSHTHI SIGN BAR ABOVE to KHAROSHTHI SIGN DOT BELOW */
    {0x10a3f, 0x10a3f},	/* KHAROSHTHI VIRAMA */
    {0x10ae5, 0x10ae6},	/* MANICHAEAN ABBREVIATION MARK ABOVE to MANICHAEAN ABBREVIATION MARK BELOW */
    {0x11001, 0x11001},	/* BRAHMI SIGN ANUSVARA */
    {0x11038, 0x11046},	/* BRAHMI VOWEL SIGN AA to BRAHMI VIRAMA */
    {0x1107f, 0x11081},	/* BRAHMI NUMBER JOINER to KAITHI SIGN ANUSVARA */
    {0x110b3, 0x110b6},	/* KAITHI VOWEL SIGN U to KAITHI VOWEL SIGN AI */
    {0x110b9, 0x110ba},	/* KAITHI SIGN VIRAMA to KAITHI SIGN NUKTA */
    {0x110bd, 0x110bd},	/* KAITHI NUMBER SIGN */
    {0x11100, 0x11102},	/* CHAKMA SIGN CANDRABINDU to CHAKMA SIGN VISARGA */
    {0x11127, 0x1112b},	/* CHAKMA VOWEL SIGN A to CHAKMA VOWEL SIGN UU */
    {0x1112d, 0x11134},	/* CHAKMA VOWEL SIGN AI to CHAKMA MAAYYAA */
    {0x11173, 0x11173},	/* MAHAJANI SIGN NUKTA */
    {0x11180, 0x11181},	/* SHARADA SIGN CANDRABINDU to SHARADA SIGN ANUSVARA */
    {0x111b6, 0x111be},	/* SHARADA VOWEL SIGN U to SHARADA VOWEL SIGN O */
    {0x111ca, 0x111cc},	/* SHARADA SIGN NUKTA to SHARADA EXTRA SHORT VOWEL MARK */
    {0x1122f, 0x11231},	/* KHOJKI VOWEL SIGN U to KHOJKI VOWEL SIGN AI */
    {0x11234, 0x11234},	/* KHOJKI SIGN ANUSVARA */
    {0x11236, 0x11237},	/* KHOJKI SIGN NUKTA to KHOJKI SIGN SHADDA */
    {0x1123e, 0x1123e},	/* KHOJKI SIGN SUKUN */
    {0x112df, 0x112df},	/* KHUDAWADI SIGN ANUSVARA */
    {0x112e3, 0x112ea},	/* KHUDAWADI VOWEL SIGN U to KHUDAWADI SIGN VIRAMA */
    {0x11300, 0x11301},	/* GRANTHA SIGN COMBINING ANUSVARA ABOVE to GRANTHA SIGN CANDRABINDU */
    {0x1133c, 0x1133c},	/* GRANTHA SIGN NUKTA */
    {0x11340, 0x11340},	/* GRANTHA VOWEL SIGN II */
    {0x11366, 0x1136c},	/* COMBINING GRANTHA DIGIT ZERO to COMBINING GRANTHA DIGIT SIX */
    {0x11370, 0x11374},	/* COMBINING GRANTHA LETTER A to COMBINING GRANTHA LETTER PA */
    {0x11438, 0x1143f},	/* NEWA VOWEL SIGN U to NEWA VOWEL SIGN AI */
    {0x11442, 0x11444},	/* NEWA SIGN VIRAMA to NEWA SIGN ANUSVARA */
    {0x11446, 0x11446},	/* NEWA SIGN NUKTA */
    {0x114b3, 0x114b8},	/* TIRHUTA VOWEL SIGN U to TIRHUTA VOWEL SIGN VOCALIC LL */
    {0x114ba, 0x114ba},	/* TIRHUTA VOWEL SIGN SHORT E */
    {0x114bf, 0x114c0},	/* TIRHUTA SIGN CANDRABINDU to TIRHUTA SIGN ANUSVARA */
    {0x114c2, 0x114c3},	/* TIRHUTA SIGN VIRAMA to TIRHUTA SIGN NUKTA */
    {0x115b2, 0x115b5},	/* SIDDHAM VOWEL SIGN U to SIDDHAM VOWEL SIGN VOCALIC RR */
    {0x115bc, 0x115bd},	/* SIDDHAM SIGN CANDRABINDU to SIDDHAM SIGN ANUSVARA */
    {0x115bf, 0x115c0},	/* SIDDHAM SIGN VIRAMA to SIDDHAM SIGN NUKTA */
    {0x115dc, 0x115dd},	/* SIDDHAM VOWEL SIGN ALTERNATE U to SIDDHAM VOWEL SIGN ALTERNATE UU */
    {0x11633, 0x1163a},	/* MODI VOWEL SIGN U to MODI VOWEL SIGN AI */
    {0x1163d, 0x1163d},	/* MODI SIGN ANUSVARA */
    {0x1163f, 0x11640},	/* MODI SIGN VIRAMA to MODI SIGN ARDHACANDRA */
    {0x116ab, 0x116ab},	/* TAKRI SIGN ANUSVARA */
    {0x116ad, 0x116ad},	/* TAKRI VOWEL SIGN AA */
    {0x116b0, 0x116b5},	/* TAKRI VOWEL SIGN U to TAKRI VOWEL SIGN AU */
    {0x116b7, 0x116b7},	/* TAKRI SIGN NUKTA */
    {0x1171d, 0x1171f},	/* AHOM CONSONANT SIGN MEDIAL LA to AHOM CONSONANT SIGN MEDIAL LIGATING RA */
    {0x11722, 0x11725},	/* AHOM VOWEL SIGN I to AHOM VOWEL SIGN UU */
    {0x11727, 0x1172b},	/* AHOM VOWEL SIGN AW to AHOM SIGN KILLER */
    {0x11a01, 0x11a06},	/* ZANABAZAR SQUARE VOWEL SIGN I to ZANABAZAR SQUARE VOWEL SIGN O */
    {0x11a09, 0x11a0a},	/* ZANABAZAR SQUARE VOWEL SIGN REVERSED I to ZANABAZAR SQUARE VOWEL LENGTH MARK */
    {0x11a33, 0x11a38},	/* ZANABAZAR SQUARE FINAL CONSONANT MARK to ZANABAZAR SQUARE SIGN ANUSVARA */
    {0x11a3b, 0x11a3e},	/* ZANABAZAR SQUARE CLUSTER-FINAL LETTER YA to ZANABAZAR SQUARE CLUSTER-FINAL LETTER VA */
    {0x11a47, 0x11a47},	/* ZANABAZAR SQUARE SUBJOINER */
    {0x11a51, 0x11a56},	/* SOYOMBO VOWEL SIGN I to SOYOMBO VOWEL SIGN OE */
    {0x11a59, 0x11a5b},	/* SOYOMBO VOWEL SIGN VOCALIC R to SOYOMBO VOWEL LENGTH MARK */
    {0x11a8a, 0x11a96},	/* SOYOMBO FINAL CONSONANT SIGN G to SOYOMBO SIGN ANUSVARA */
    {0x11a98, 0x11a99},	/* SOYOMBO GEMINATION MARK to SOYOMBO SUBJOINER */
    {0x11c30, 0x11c36},	/* BHAIKSUKI VOWEL SIGN I to BHAIKSUKI VOWEL SIGN VOCALIC L */
    {0x11c38, 0x11c3d},	/* BHAIKSUKI VOWEL SIGN E to BHAIKSUKI SIGN ANUSVARA */
    {0x11c3f, 0x11c3f},	/* BHAIKSUKI SIGN VIRAMA */
    {0x11c92, 0x11ca7},	/* MARCHEN SUBJOINED LETTER KA to MARCHEN SUBJOINED LETTER ZA */
    {0x11caa, 0x11cb0},	/* MARCHEN SUBJOINED LETTER RA to MARCHEN VOWEL SIGN AA */
    {0x11cb2, 0x11cb3},	/* MARCHEN VOWEL SIGN U to MARCHEN VOWEL SIGN E */
    {0x11cb5, 0x11cb6},	/* MARCHEN SIGN ANUSVARA to MARCHEN SIGN CANDRABINDU */
    {0x11d31, 0x11d36},	/* MASARAM GONDI VOWEL SIGN AA to MASARAM GONDI VOWEL SIGN VOCALIC R */
    {0x11d3a, 0x11d3a},	/* MASARAM GONDI VOWEL SIGN E */
    {0x11d3c, 0x11d3d},	/* MASARAM GONDI VOWEL SIGN AI to MASARAM GONDI VOWEL SIGN O */
    {0x11d3f, 0x11d45},	/* MASARAM GONDI VOWEL SIGN AU to MASARAM GONDI VIRAMA */
    {0x11d47, 0x11d47},	/* MASARAM GONDI RA-KARA */
    {0x16af0, 0x16af4},	/* BASSA VAH COMBINING HIGH TONE to BASSA VAH COMBINING HIGH-LOW TONE */
    {0x16b30, 0x16b36},	/* PAHAWH HMONG MARK CIM TUB to PAHAWH HMONG MARK CIM TAUM */
    {0x16f8f, 0x16f92},	/* MIAO TONE RIGHT to MIAO TONE BELOW */
    {0x1bc9d, 0x1bc9e},	/* DUPLOYAN THICK LETTER SELECTOR to DUPLOYAN DOUBLE MARK */
    {0x1bca0, 0x1bca3},	/* SHORTHAND FORMAT LETTER OVERLAP to SHORTHAND FORMAT UP STEP */
    {0x1d167, 0x1d169},	/* MUSICAL SYMBOL COMBINING TREMOLO-1 to MUSICAL SYMBOL COMBINING TREMOLO-3 */
    {0x1d173, 0x1d182},	/* MUSICAL SYMBOL BEGIN BEAM to MUSICAL SYMBOL COMBINING LOURE */
    {0x1d185, 0x1d18b},	/* MUSICAL SYMBOL COMBINING DOIT to MUSICAL SYMBOL COMBINING TRIPLE TONGUE */
    {0x1d1aa, 0x1d1ad},	/* MUSICAL SYMBOL COMBINING DOWN BOW to MUSICAL SYMBOL COMBINING SNAP PIZZICATO */
    {0x1d242, 0x1d244},	/* COMBINING GREEK MUSICAL TRISEME to COMBINING GREEK MUSICAL PENTASEME */
    {0x1da00, 0x1da36},	/* SIGNWRITING HEAD RIM to SIGNWRITING AIR SUCKING IN */
    {0x1da3b, 0x1da6c},	/* SIGNWRITING MOUTH CLOSED NEUTRAL to SIGNWRITING EXCITEMENT */
    {0x1da75, 0x1da75},	/* SIGNWRITING UPPER BODY TILTING FROM HIP JOINTS */
    {0x1da84, 0x1da84},	/* SIGNWRITING LOCATION HEAD NECK */
    {0x1da9b, 0x1da9f},	/* SIGNWRITING FILL MODIFIER-2 to SIGNWRITING FILL MODIFIER-6 */
    {0x1daa1, 0x1daaf},	/* SIGNWRITING ROTATION MODIFIER-2 to SIGNWRITING ROTATION MODIFIER-16 */
    {0x1e000, 0x1e006},	/* COMBINING GLAGOLITIC LETTER AZU to COMBINING GLAGOLITIC LETTER ZHIVETE */
    {0x1e008, 0x1e018},	/* COMBINING GLAGOLITIC LETTER ZEMLJA to COMBINING GLAGOLITIC LETTER HERU */
    {0x1e01b, 0x1e021},	/* COMBINING GLAGOLITIC LETTER SHTA to COMBINING GLAGOLITIC LETTER YATI */
    {0x1e023, 0x1e024},	/* COMBINING GLAGOLITIC LETTER YU to COMBINING GLAGOLITIC LETTER SMALL YUS */
    {0x1e026, 0x1e02a},	/* COMBINING GLAGOLITIC LETTER YO to COMBINING GLAGOLITIC LETTER FITA */
    {0x1e8d0, 0x1e8d6},	/* MENDE KIKAKUI COMBINING NUMBER TEENS to MENDE KIKAKUI COMBINING NUMBER MILLIONS */
    {0x1e944, 0x1e94a},	/* ADLAM ALIF LENGTHENER to ADLAM NUKTA */
    {0xe0001, 0xe0001},	/* LANGUAGE TAG */
    {0xe0020, 0xe007f},	/* TAG SPACE to CANCEL TAG */
    {0xe0100, 0xe01ef},	/* VARIATION SELECTOR-17 to VARIATION SELECTOR-256 */
};

  /* sorted list of non-overlapping intervals of wide characters */
  /* All 'W' and 'F' characters from version 10.0.0 of
     http://www.unicode.org/Public/UNIDATA/EastAsianWidth.txt
   */

  static const struct interval wide[] = {
    {0x1100, 0x115f},	/* HANGUL CHOSEONG KIYEOK to HANGUL CHOSEONG FILLER */
    {0x231a, 0x231b},	/* WATCH to HOURGLASS */
    {0x2329, 0x232a},	/* LEFT-POINTING ANGLE BRACKET to RIGHT-POINTING ANGLE BRACKET */
    {0x23e9, 0x23ec},	/* BLACK RIGHT-POINTING DOUBLE TRIANGLE to BLACK DOWN-POINTING DOUBLE TRIANGLE */
    {0x23f0, 0x23f0},	/* ALARM CLOCK */
    {0x23f3, 0x23f3},	/* HOURGLASS WITH FLOWING SAND */
    {0x25fd, 0x25fe},	/* WHITE MEDIUM SMALL SQUARE to BLACK MEDIUM SMALL SQUARE */
    {0x2614, 0x2615},	/* UMBRELLA WITH RAIN DROPS to HOT BEVERAGE */
    {0x2648, 0x2653},	/* ARIES to PISCES */
    {0x267f, 0x267f},	/* WHEELCHAIR SYMBOL */
    {0x2693, 0x2693},	/* ANCHOR */
    {0x26a1, 0x26a1},	/* HIGH VOLTAGE SIGN */
    {0x26aa, 0x26ab},	/* MEDIUM WHITE CIRCLE to MEDIUM BLACK CIRCLE */
    {0x26bd, 0x26be},	/* SOCCER BALL to BASEBALL */
    {0x26c4, 0x26c5},	/* SNOWMAN WITHOUT SNOW to SUN BEHIND CLOUD */
    {0x26ce, 0x26ce},	/* OPHIUCHUS */
    {0x26d4, 0x26d4},	/* NO ENTRY */
    {0x26ea, 0x26ea},	/* CHURCH */
    {0x26f2, 0x26f3},	/* FOUNTAIN to FLAG IN HOLE */
    {0x26f5, 0x26f5},	/* SAILBOAT */
    {0x26fa, 0x26fa},	/* TENT */
    {0x26fd, 0x26fd},	/* FUEL PUMP */
    {0x2705, 0x2705},	/* WHITE HEAVY CHECK MARK */
    {0x270a, 0x270b},	/* RAISED FIST to RAISED HAND */
    {0x2728, 0x2728},	/* SPARKLES */
    {0x274c, 0x274c},	/* CROSS MARK */
    {0x274e, 0x274e},	/* NEGATIVE SQUARED CROSS MARK */
    {0x2753, 0x2755},	/* BLACK QUESTION MARK ORNAMENT to WHITE EXCLAMATION MARK ORNAMENT */
    {0x2757, 0x2757},	/* HEAVY EXCLAMATION MARK SYMBOL */
    {0x2795, 0x2797},	/* HEAVY PLUS SIGN to HEAVY DIVISION SIGN */
    {0x27b0, 0x27b0},	/* CURLY LOOP */
    {0x27bf, 0x27bf},	/* DOUBLE CURLY LOOP */
    {0x2b1b, 0x2b1c},	/* BLACK LARGE SQUARE to WHITE LARGE SQUARE */
    {0x2b50, 0x2b50},	/* WHITE MEDIUM STAR */
    {0x2b55, 0x2b55},	/* HEAVY LARGE CIRCLE */
    {0x2e80, 0x2e99},	/* CJK RADICAL REPEAT to CJK RADICAL RAP */
    {0x2e9b, 0x2ef3},	/* CJK RADICAL CHOKE to CJK RADICAL C-SIMPLIFIED TURTLE */
    {0x2f00, 0x2fd5},	/* KANGXI RADICAL ONE to KANGXI RADICAL FLUTE */
    {0x2ff0, 0x2ffb},	/* IDEOGRAPHIC DESCRIPTION CHARACTER LEFT TO RIGHT to IDEOGRAPHIC DESCRIPTION CHARACTER OVERLAID */
    {0x3000, 0x303e},	/* IDEOGRAPHIC SPACE to IDEOGRAPHIC VARIATION INDICATOR */
    {0x3041, 0x3096},	/* HIRAGANA LETTER SMALL A to HIRAGANA LETTER SMALL KE */
    {0x3099, 0x30ff},	/* COMBINING KATAKANA-HIRAGANA VOICED SOUND MARK to KATAKANA DIGRAPH KOTO */
    {0x3105, 0x312e},	/* BOPOMOFO LETTER B to BOPOMOFO LETTER O WITH DOT ABOVE */
    {0x3131, 0x318e},	/* HANGUL LETTER KIYEOK to HANGUL LETTER ARAEAE */
    {0x3190, 0x31ba},	/* IDEOGRAPHIC ANNOTATION LINKING MARK to BOPOMOFO LETTER ZY */
    {0x31c0, 0x31e3},	/* CJK STROKE T to CJK STROKE Q */
    {0x31f0, 0x321e},	/* KATAKANA LETTER SMALL KU to PARENTHESIZED KOREAN CHARACTER O HU */
    {0x3220, 0x3247},	/* PARENTHESIZED IDEOGRAPH ONE to CIRCLED IDEOGRAPH KOTO */
    {0x3250, 0x32fe},	/* PARTNERSHIP SIGN to CIRCLED KATAKANA WO */
    {0x3300, 0x4dbf},	/* SQUARE APAATO to  */
    {0x4e00, 0xa48c},	/* <CJK Ideograph, First> to YI SYLLABLE YYR */
    {0xa490, 0xa4c6},	/* YI RADICAL QOT to YI RADICAL KE */
    {0xa960, 0xa97c},	/* HANGUL CHOSEONG TIKEUT-MIEUM to HANGUL CHOSEONG SSANGYEORINHIEUH */
    {0xac00, 0xd7a3},	/* <Hangul Syllable, First> to <Hangul Syllable, Last> */
    {0xf900, 0xfaff},	/* CJK COMPATIBILITY IDEOGRAPH-F900 to  */
    {0xfe10, 0xfe19},	/* PRESENTATION FORM FOR VERTICAL COMMA to PRESENTATION FORM FOR VERTICAL HORIZONTAL ELLIPSIS */
    {0xfe30, 0xfe52},	/* PRESENTATION FORM FOR VERTICAL TWO DOT LEADER to SMALL FULL STOP */
    {0xfe54, 0xfe66},	/* SMALL SEMICOLON to SMALL EQUALS SIGN */
    {0xfe68, 0xfe6b},	/* SMALL REVERSE SOLIDUS to SMALL COMMERCIAL AT */
    {0xff01, 0xff60},	/* FULLWIDTH EXCLAMATION MARK to FULLWIDTH RIGHT WHITE PARENTHESIS */
    {0xffe0, 0xffe6},	/* FULLWIDTH CENT SIGN to FULLWIDTH WON SIGN */
    {0x16fe0, 0x16fe1},	/* TANGUT ITERATION MARK to NUSHU ITERATION MARK */
    {0x17000, 0x187ec},	/* <Tangut Ideograph, First> to <Tangut Ideograph, Last> */
    {0x18800, 0x18af2},	/* TANGUT COMPONENT-001 to TANGUT COMPONENT-755 */
    {0x1b000, 0x1b11e},	/* KATAKANA LETTER ARCHAIC E to HENTAIGANA LETTER N-MU-MO-2 */
    {0x1b170, 0x1b2fb},	/* NUSHU CHARACTER-1B170 to NUSHU CHARACTER-1B2FB */
    {0x1f004, 0x1f004},	/* MAHJONG TILE RED DRAGON */
    {0x1f0cf, 0x1f0cf},	/* PLAYING CARD BLACK JOKER */
    {0x1f18e, 0x1f18e},	/* NEGATIVE SQUARED AB */
    {0x1f191, 0x1f19a},	/* SQUARED CL to SQUARED VS */
    {0x1f200, 0x1f202},	/* SQUARE HIRAGANA HOKA to SQUARED KATAKANA SA */
    {0x1f210, 0x1f23b},	/* SQUARED CJK UNIFIED IDEOGRAPH-624B to SQUARED CJK UNIFIED IDEOGRAPH-914D */
    {0x1f240, 0x1f248},	/* TORTOISE SHELL BRACKETED CJK UNIFIED IDEOGRAPH-672C to TORTOISE SHELL BRACKETED CJK UNIFIED IDEOGRAPH-6557 */
    {0x1f250, 0x1f251},	/* CIRCLED IDEOGRAPH ADVANTAGE to CIRCLED IDEOGRAPH ACCEPT */
    {0x1f260, 0x1f265},	/* ROUNDED SYMBOL FOR FU to ROUNDED SYMBOL FOR CAI */
    {0x1f300, 0x1f320},	/* CYCLONE to SHOOTING STAR */
    {0x1f32d, 0x1f335},	/* HOT DOG to CACTUS */
    {0x1f337, 0x1f37c},	/* TULIP to BABY BOTTLE */
    {0x1f37e, 0x1f393},	/* BOTTLE WITH POPPING CORK to GRADUATION CAP */
    {0x1f3a0, 0x1f3ca},	/* CAROUSEL HORSE to SWIMMER */
    {0x1f3cf, 0x1f3d3},	/* CRICKET BAT AND BALL to TABLE TENNIS PADDLE AND BALL */
    {0x1f3e0, 0x1f3f0},	/* HOUSE BUILDING to EUROPEAN CASTLE */
    {0x1f3f4, 0x1f3f4},	/* WAVING BLACK FLAG */
    {0x1f3f8, 0x1f43e},	/* BADMINTON RACQUET AND SHUTTLECOCK to PAW PRINTS */
    {0x1f440, 0x1f440},	/* EYES */
    {0x1f442, 0x1f4fc},	/* EAR to VIDEOCASSETTE */
    {0x1f4ff, 0x1f53d},	/* PRAYER BEADS to DOWN-POINTING SMALL RED TRIANGLE */
    {0x1f54b, 0x1f54e},	/* KAABA to MENORAH WITH NINE BRANCHES */
    {0x1f550, 0x1f567},	/* CLOCK FACE ONE OCLOCK to CLOCK FACE TWELVE-THIRTY */
    {0x1f57a, 0x1f57a},	/* MAN DANCING */
    {0x1f595, 0x1f596},	/* REVERSED HAND WITH MIDDLE FINGER EXTENDED to RAISED HAND WITH PART BETWEEN MIDDLE AND RING FINGERS */
    {0x1f5a4, 0x1f5a4},	/* BLACK HEART */
    {0x1f5fb, 0x1f64f},	/* MOUNT FUJI to PERSON WITH FOLDED HANDS */
    {0x1f680, 0x1f6c5},	/* ROCKET to LEFT LUGGAGE */
    {0x1f6cc, 0x1f6cc},	/* SLEEPING ACCOMMODATION */
    {0x1f6d0, 0x1f6d2},	/* PLACE OF WORSHIP to SHOPPING TROLLEY */
    {0x1f6eb, 0x1f6ec},	/* AIRPLANE DEPARTURE to AIRPLANE ARRIVING */
    {0x1f6f4, 0x1f6f8},	/* SCOOTER to FLYING SAUCER */
    {0x1f910, 0x1f93e},	/* ZIPPER-MOUTH FACE to HANDBALL */
    {0x1f940, 0x1f94c},	/* WILTED FLOWER to CURLING STONE */
    {0x1f950, 0x1f96b},	/* CROISSANT to CANNED FOOD */
    {0x1f980, 0x1f997},	/* CRAB to CRICKET */
    {0x1f9c0, 0x1f9c0},	/* CHEESE WEDGE */
    {0x1f9d0, 0x1f9e6},	/* FACE WITH MONOCLE to SOCKS */
    {0x20000, 0x2fffd},	/* <CJK Ideograph Extension B, First> to  */
    {0x30000, 0x3fffd},	/*  to  */
};

  /* Fast test for 8-bit control characters and many ISO8859 characters. */
  /* NOTE: this overrides the 'Cf' definition of the U+00AD character */
  if (ucs < 0x0300) {
    if (ucs == 0)
      return 0;
    if (ucs < 32 || (ucs >= 0x7f && ucs < 0xa0))
      return -1;
    return 1;
  }

  /* binary search in table of non-spacing characters */
  if (bisearch(ucs, combining,
	       sizeof(combining) / sizeof(struct interval) - 1))
    return 0;

  /* The first wide character is U+1100, everything below it is 'normal'. */
  if (ucs < 0x1100)
    return 1;

  /* Hangul Jamo medial vowels and final consonants (U+1160-U+11FF)
   * are zero length despite not being Mn, Me or Cf */
  if (ucs >= 0x1160 && ucs <= 0x11FF)
    return 0;

  /* if we arrive here, ucs is not a combining or C0/C1 control character */
  return 1 + (bisearch(ucs, wide, sizeof(wide) / sizeof(struct interval) - 1));
}


int mk_wcswidth(const unsigned int *pwcs, size_t n)
{
  int w, width = 0;

  for (;*pwcs && n-- > 0; pwcs++)
    if ((w = mk_wcwidth(*pwcs)) < 0)
      return -1;
    else
      width += w;

  return width;
}


/*
 * The following functions are the same as mk_wcwidth() and
 * mk_wcswidth(), except that spacing characters in the East Asian
 * Ambiguous (A) category as defined in Unicode Technical Report #11
 * have a column width of 2. This variant might be useful for users of
 * CJK legacy encodings who want to migrate to UCS without changing
 * the traditional terminal character-width behaviour. It is not
 * otherwise recommended for general use.
 */
int mk_wcwidth_cjk(unsigned int ucs)
{
  /* sorted list of non-overlapping intervals of East Asian Ambiguous
   * characters. */
  /* All 'A' characters from version 10.0.0 of
     http://www.unicode.org/Public/UNIDATA/EastAsianWidth.txt
   */
  static const struct interval ambiguous[] = {
    {0x00a1, 0x00a1},	/* INVERTED EXCLAMATION MARK */
    {0x00a4, 0x00a4},	/* CURRENCY SIGN */
    {0x00a7, 0x00a8},	/* SECTION SIGN to DIAERESIS */
    {0x00aa, 0x00aa},	/* FEMININE ORDINAL INDICATOR */
    {0x00ad, 0x00ae},	/* SOFT HYPHEN to REGISTERED SIGN */
    {0x00b0, 0x00b4},	/* DEGREE SIGN to ACUTE ACCENT */
    {0x00b6, 0x00ba},	/* PILCROW SIGN to MASCULINE ORDINAL INDICATOR */
    {0x00bc, 0x00bf},	/* VULGAR FRACTION ONE QUARTER to INVERTED QUESTION MARK */
    {0x00c6, 0x00c6},	/* LATIN CAPITAL LETTER AE */
    {0x00d0, 0x00d0},	/* LATIN CAPITAL LETTER ETH */
    {0x00d7, 0x00d8},	/* MULTIPLICATION SIGN to LATIN CAPITAL LETTER O WITH STROKE */
    {0x00de, 0x00e1},	/* LATIN CAPITAL LETTER THORN to LATIN SMALL LETTER A WITH ACUTE */
    {0x00e6, 0x00e6},	/* LATIN SMALL LETTER AE */
    {0x00e8, 0x00ea},	/* LATIN SMALL LETTER E WITH GRAVE to LATIN SMALL LETTER E WITH CIRCUMFLEX */
    {0x00ec, 0x00ed},	/* LATIN SMALL LETTER I WITH GRAVE to LATIN SMALL LETTER I WITH ACUTE */
    {0x00f0, 0x00f0},	/* LATIN SMALL LETTER ETH */
    {0x00f2, 0x00f3},	/* LATIN SMALL LETTER O WITH GRAVE to LATIN SMALL LETTER O WITH ACUTE */
    {0x00f7, 0x00fa},	/* DIVISION SIGN to LATIN SMALL LETTER U WITH ACUTE */
    {0x00fc, 0x00fc},	/* LATIN SMALL LETTER U WITH DIAERESIS */
    {0x00fe, 0x00fe},	/* LATIN SMALL LETTER THORN */
    {0x0101, 0x0101},	/* LATIN SMALL LETTER A WITH MACRON */
    {0x0111, 0x0111},	/* LATIN SMALL LETTER D WITH STROKE */
    {0x0113, 0x0113},	/* LATIN SMALL LETTER E WITH MACRON */
    {0x011b, 0x011b},	/* LATIN SMALL LETTER E WITH CARON */
    {0x0126, 0x0127},	/* LATIN CAPITAL LETTER H WITH STROKE to LATIN SMALL LETTER H WITH STROKE */
    {0x012b, 0x012b},	/* LATIN SMALL LETTER I WITH MACRON */
    {0x0131, 0x0133},	/* LATIN SMALL LETTER DOTLESS I to LATIN SMALL LIGATURE IJ */
    {0x0138, 0x0138},	/* LATIN SMALL LETTER KRA */
    {0x013f, 0x0142},	/* LATIN CAPITAL LETTER L WITH MIDDLE DOT to LATIN SMALL LETTER L WITH STROKE */
    {0x0144, 0x0144},	/* LATIN SMALL LETTER N WITH ACUTE */
    {0x0148, 0x014b},	/* LATIN SMALL LETTER N WITH CARON to LATIN SMALL LETTER ENG */
    {0x014d, 0x014d},	/* LATIN SMALL LETTER O WITH MACRON */
    {0x0152, 0x0153},	/* LATIN CAPITAL LIGATURE OE to LATIN SMALL LIGATURE OE */
    {0x0166, 0x0167},	/* LATIN CAPITAL LETTER T WITH STROKE to LATIN SMALL LETTER T WITH STROKE */
    {0x016b, 0x016b},	/* LATIN SMALL LETTER U WITH MACRON */
    {0x01ce, 0x01ce},	/* LATIN SMALL LETTER A WITH CARON */
    {0x01d0, 0x01d0},	/* LATIN SMALL LETTER I WITH CARON */
    {0x01d2, 0x01d2},	/* LATIN SMALL LETTER O WITH CARON */
    {0x01d4, 0x01d4},	/* LATIN SMALL LETTER U WITH CARON */
    {0x01d6, 0x01d6},	/* LATIN SMALL LETTER U WITH DIAERESIS AND MACRON */
    {0x01d8, 0x01d8},	/* LATIN SMALL LETTER U WITH DIAERESIS AND ACUTE */
    {0x01da, 0x01da},	/* LATIN SMALL LETTER U WITH DIAERESIS AND CARON */
    {0x01dc, 0x01dc},	/* LATIN SMALL LETTER U WITH DIAERESIS AND GRAVE */
    {0x0251, 0x0251},	/* LATIN SMALL LETTER ALPHA */
    {0x0261, 0x0261},	/* LATIN SMALL LETTER SCRIPT G */
    {0x02c4, 0x02c4},	/* MODIFIER LETTER UP ARROWHEAD */
    {0x02c7, 0x02c7},	/* CARON */
    {0x02c9, 0x02cb},	/* MODIFIER LETTER MACRON to MODIFIER LETTER GRAVE ACCENT */
    {0x02cd, 0x02cd},	/* MODIFIER LETTER LOW MACRON */
    {0x02d0, 0x02d0},	/* MODIFIER LETTER TRIANGULAR COLON */
    {0x02d8, 0x02db},	/* BREVE to OGONEK */
    {0x02dd, 0x02dd},	/* DOUBLE ACUTE ACCENT */
    {0x02df, 0x02df},	/* MODIFIER LETTER CROSS ACCENT */
    {0x0300, 0x036f},	/* COMBINING GRAVE ACCENT to COMBINING LATIN SMALL LETTER X */
    {0x0391, 0x03a1},	/* GREEK CAPITAL LETTER ALPHA to GREEK CAPITAL LETTER RHO */
    {0x03a3, 0x03a9},	/* GREEK CAPITAL LETTER SIGMA to GREEK CAPITAL LETTER OMEGA */
    {0x03b1, 0x03c1},	/* GREEK SMALL LETTER ALPHA to GREEK SMALL LETTER RHO */
    {0x03c3, 0x03c9},	/* GREEK SMALL LETTER SIGMA to GREEK SMALL LETTER OMEGA */
    {0x0401, 0x0401},	/* CYRILLIC CAPITAL LETTER IO */
    {0x0410, 0x044f},	/* CYRILLIC CAPITAL LETTER A to CYRILLIC SMALL LETTER YA */
    {0x0451, 0x0451},	/* CYRILLIC SMALL LETTER IO */
    {0x2010, 0x2010},	/* HYPHEN */
    {0x2013, 0x2016},	/* EN DASH to DOUBLE VERTICAL LINE */
    {0x2018, 0x2019},	/* LEFT SINGLE QUOTATION MARK to RIGHT SINGLE QUOTATION MARK */
    {0x201c, 0x201d},	/* LEFT DOUBLE QUOTATION MARK to RIGHT DOUBLE QUOTATION MARK */
    {0x2020, 0x2022},	/* DAGGER to BULLET */
    {0x2024, 0x2027},	/* ONE DOT LEADER to HYPHENATION POINT */
    {0x2030, 0x2030},	/* PER MILLE SIGN */
    {0x2032, 0x2033},	/* PRIME to DOUBLE PRIME */
    {0x2035, 0x2035},	/* REVERSED PRIME */
    {0x203b, 0x203b},	/* REFERENCE MARK */
    {0x203e, 0x203e},	/* OVERLINE */
    {0x2074, 0x2074},	/* SUPERSCRIPT FOUR */
    {0x207f, 0x207f},	/* SUPERSCRIPT LATIN SMALL LETTER N */
    {0x2081, 0x2084},	/* SUBSCRIPT ONE to SUBSCRIPT FOUR */
    {0x20ac, 0x20ac},	/* EURO SIGN */
    {0x2103, 0x2103},	/* DEGREE CELSIUS */
    {0x2105, 0x2105},	/* CARE OF */
    {0x2109, 0x2109},	/* DEGREE FAHRENHEIT */
    {0x2113, 0x2113},	/* SCRIPT SMALL L */
    {0x2116, 0x2116},	/* NUMERO SIGN */
    {0x2121, 0x2122},	/* TELEPHONE SIGN to TRADE MARK SIGN */
    {0x2126, 0x2126},	/* OHM SIGN */
    {0x212b, 0x212b},	/* ANGSTROM SIGN */
    {0x2153, 0x2154},	/* VULGAR FRACTION ONE THIRD to VULGAR FRACTION TWO THIRDS */
    {0x215b, 0x215e},	/* VULGAR FRACTION ONE EIGHTH to VULGAR FRACTION SEVEN EIGHTHS */
    {0x2160, 0x216b},	/* ROMAN NUMERAL ONE to ROMAN NUMERAL TWELVE */
    {0x2170, 0x2179},	/* SMALL ROMAN NUMERAL ONE to SMALL ROMAN NUMERAL TEN */
    {0x2189, 0x2189},	/* VULGAR FRACTION ZERO THIRDS */
    {0x2190, 0x2199},	/* LEFTWARDS ARROW to SOUTH WEST ARROW */
    {0x21b8, 0x21b9},	/* NORTH WEST ARROW TO LONG BAR to LEFTWARDS ARROW TO BAR OVER RIGHTWARDS ARROW TO BAR */
    {0x21d2, 0x21d2},	/* RIGHTWARDS DOUBLE ARROW */
    {0x21d4, 0x21d4},	/* LEFT RIGHT DOUBLE ARROW */
    {0x21e7, 0x21e7},	/* UPWARDS WHITE ARROW */
    {0x2200, 0x2200},	/* FOR ALL */
    {0x2202, 0x2203},	/* PARTIAL DIFFERENTIAL to THERE EXISTS */
    {0x2207, 0x2208},	/* NABLA to ELEMENT OF */
    {0x220b, 0x220b},	/* CONTAINS AS MEMBER */
    {0x220f, 0x220f},	/* N-ARY PRODUCT */
    {0x2211, 0x2211},	/* N-ARY SUMMATION */
    {0x2215, 0x2215},	/* DIVISION SLASH */
    {0x221a, 0x221a},	/* SQUARE ROOT */
    {0x221d, 0x2220},	/* PROPORTIONAL TO to ANGLE */
    {0x2223, 0x2223},	/* DIVIDES */
    {0x2225, 0x2225},	/* PARALLEL TO */
    {0x2227, 0x222c},	/* LOGICAL AND to DOUBLE INTEGRAL */
    {0x222e, 0x222e},	/* CONTOUR INTEGRAL */
    {0x2234, 0x2237},	/* THEREFORE to PROPORTION */
    {0x223c, 0x223d},	/* TILDE OPERATOR to REVERSED TILDE */
    {0x2248, 0x2248},	/* ALMOST EQUAL TO */
    {0x224c, 0x224c},	/* ALL EQUAL TO */
    {0x2252, 0x2252},	/* APPROXIMATELY EQUAL TO OR THE IMAGE OF */
    {0x2260, 0x2261},	/* NOT EQUAL TO to IDENTICAL TO */
    {0x2264, 0x2267},	/* LESS-THAN OR EQUAL TO to GREATER-THAN OVER EQUAL TO */
    {0x226a, 0x226b},	/* MUCH LESS-THAN to MUCH GREATER-THAN */
    {0x226e, 0x226f},	/* NOT LESS-THAN to NOT GREATER-THAN */
    {0x2282, 0x2283},	/* SUBSET OF to SUPERSET OF */
    {0x2286, 0x2287},	/* SUBSET OF OR EQUAL TO to SUPERSET OF OR EQUAL TO */
    {0x2295, 0x2295},	/* CIRCLED PLUS */
    {0x2299, 0x2299},	/* CIRCLED DOT OPERATOR */
    {0x22a5, 0x22a5},	/* UP TACK */
    {0x22bf, 0x22bf},	/* RIGHT TRIANGLE */
    {0x2312, 0x2312},	/* ARC */
    {0x2460, 0x24e9},	/* CIRCLED DIGIT ONE to CIRCLED LATIN SMALL LETTER Z */
    {0x24eb, 0x254b},	/* NEGATIVE CIRCLED NUMBER ELEVEN to BOX DRAWINGS HEAVY VERTICAL AND HORIZONTAL */
    {0x2550, 0x2573},	/* BOX DRAWINGS DOUBLE HORIZONTAL to BOX DRAWINGS LIGHT DIAGONAL CROSS */
    {0x2580, 0x258f},	/* UPPER HALF BLOCK to LEFT ONE EIGHTH BLOCK */
    {0x2592, 0x2595},	/* MEDIUM SHADE to RIGHT ONE EIGHTH BLOCK */
    {0x25a0, 0x25a1},	/* BLACK SQUARE to WHITE SQUARE */
    {0x25a3, 0x25a9},	/* WHITE SQUARE CONTAINING BLACK SMALL SQUARE to SQUARE WITH DIAGONAL CROSSHATCH FILL */
    {0x25b2, 0x25b3},	/* BLACK UP-POINTING TRIANGLE to WHITE UP-POINTING TRIANGLE */
    {0x25b6, 0x25b7},	/* BLACK RIGHT-POINTING TRIANGLE to WHITE RIGHT-POINTING TRIANGLE */
    {0x25bc, 0x25bd},	/* BLACK DOWN-POINTING TRIANGLE to WHITE DOWN-POINTING TRIANGLE */
    {0x25c0, 0x25c1},	/* BLACK LEFT-POINTING TRIANGLE to WHITE LEFT-POINTING TRIANGLE */
    {0x25c6, 0x25c8},	/* BLACK DIAMOND to WHITE DIAMOND CONTAINING BLACK SMALL DIAMOND */
    {0x25cb, 0x25cb},	/* WHITE CIRCLE */
    {0x25ce, 0x25d1},	/* BULLSEYE to CIRCLE WITH RIGHT HALF BLACK */
    {0x25e2, 0x25e5},	/* BLACK LOWER RIGHT TRIANGLE to BLACK UPPER RIGHT TRIANGLE */
    {0x25ef, 0x25ef},	/* LARGE CIRCLE */
    {0x2605, 0x2606},	/* BLACK STAR to WHITE STAR */
    {0x2609, 0x2609},	/* SUN */
    {0x260e, 0x260f},	/* BLACK TELEPHONE to WHITE TELEPHONE */
    {0x261c, 0x261c},	/* WHITE LEFT POINTING INDEX */
    {0x261e, 0x261e},	/* WHITE RIGHT POINTING INDEX */
    {0x2640, 0x2640},	/* FEMALE SIGN */
    {0x2642, 0x2642},	/* MALE SIGN */
    {0x2660, 0x2661},	/* BLACK SPADE SUIT to WHITE HEART SUIT */
    {0x2663, 0x2665},	/* BLACK CLUB SUIT to BLACK HEART SUIT */
    {0x2667, 0x266a},	/* WHITE CLUB SUIT to EIGHTH NOTE */
    {0x266c, 0x266d},	/* BEAMED SIXTEENTH NOTES to MUSIC FLAT SIGN */
    {0x266f, 0x266f},	/* MUSIC SHARP SIGN */
    {0x269e, 0x269f},	/* THREE LINES CONVERGING RIGHT to THREE LINES CONVERGING LEFT */
    {0x26bf, 0x26bf},	/* SQUARED KEY */
    {0x26c6, 0x26cd},	/* RAIN to DISABLED CAR */
    {0x26cf, 0x26d3},	/* PICK to CHAINS */
    {0x26d5, 0x26e1},	/* ALTERNATE ONE-WAY LEFT WAY TRAFFIC to RESTRICTED LEFT ENTRY-2 */
    {0x26e3, 0x26e3},	/* HEAVY CIRCLE WITH STROKE AND TWO DOTS ABOVE */
    {0x26e8, 0x26e9},	/* BLACK CROSS ON SHIELD to SHINTO SHRINE */
    {0x26eb, 0x26f1},	/* CASTLE to UMBRELLA ON GROUND */
    {0x26f4, 0x26f4},	/* FERRY */
    {0x26f6, 0x26f9},	/* SQUARE FOUR CORNERS to PERSON WITH BALL */
    {0x26fb, 0x26fc},	/* JAPANESE BANK SYMBOL to HEADSTONE GRAVEYARD SYMBOL */
    {0x26fe, 0x26ff},	/* CUP ON BLACK SQUARE to WHITE FLAG WITH HORIZONTAL MIDDLE BLACK STRIPE */
    {0x273d, 0x273d},	/* HEAVY TEARDROP-SPOKED ASTERISK */
    {0x2776, 0x277f},	/* DINGBAT NEGATIVE CIRCLED DIGIT ONE to DINGBAT NEGATIVE CIRCLED NUMBER TEN */
    {0x2b56, 0x2b59},	/* HEAVY OVAL WITH OVAL INSIDE to HEAVY CIRCLED SALTIRE */
    {0x3248, 0x324f},	/* CIRCLED NUMBER TEN ON BLACK SQUARE to CIRCLED NUMBER EIGHTY ON BLACK SQUARE */
    {0xe000, 0xf8ff},	/* <Private Use, First> to <Private Use, Last> */
    {0xfe00, 0xfe0f},	/* VARIATION SELECTOR-1 to VARIATION SELECTOR-16 */
    {0xfffd, 0xfffd},	/* REPLACEMENT CHARACTER */
    {0x1f100, 0x1f10a},	/* DIGIT ZERO FULL STOP to DIGIT NINE COMMA */
    {0x1f110, 0x1f12d},	/* PARENTHESIZED LATIN CAPITAL LETTER A to CIRCLED CD */
    {0x1f130, 0x1f169},	/* SQUARED LATIN CAPITAL LETTER A to NEGATIVE CIRCLED LATIN CAPITAL LETTER Z */
    {0x1f170, 0x1f18d},	/* NEGATIVE SQUARED LATIN CAPITAL LETTER A to NEGATIVE SQUARED SA */
    {0x1f18f, 0x1f190},	/* NEGATIVE SQUARED WC to SQUARE DJ */
    {0x1f19b, 0x1f1ac},	/* SQUARED THREE D to SQUARED VOD */
    {0xe0100, 0xe01ef},	/* VARIATION SELECTOR-17 to VARIATION SELECTOR-256 */
    {0xf0000, 0xffffd},	/* <Plane 15 Private Use, First> to <Plane 15 Private Use, Last> */
    {0x100000, 0x10fffd},	/* <Plane 16 Private Use, First> to <Plane 16 Private Use, Last> */
  };
  int w = mk_wcwidth(ucs);
  if (w != 1 || ucs < 128) return w;

  /* binary search in table of non-spacing characters */
  if (bisearch(ucs, ambiguous,
	       sizeof(ambiguous) / sizeof(struct interval) - 1))
    return 2;
  return 1;
}


int mk_wcswidth_cjk(const unsigned int *pwcs, size_t n)
{
  int w, width = 0;

  for (;*pwcs && n-- > 0; pwcs++)
    if ((w = mk_wcwidth_cjk(*pwcs)) < 0)
      return -1;
    else
      width += w;

  return width;
}
