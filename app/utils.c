/**
 * \file    utils.c
 * \brief   support functions
 * \author  Yunhui Fu <yhfudev@gmail.com>
 * \version 1.0
 */

#include <string.h>
#include <stdlib.h> // free()
#include <ctype.h> // isblank()
#include <assert.h>

#include "utils.h"

#define DBGMSG(a,b,...)

#if DEBUG
#include "hexdump.h"
#else
#define hex_dump_to_fd(fd, fragment, size)
#endif

/**
 * \brief 折半方式查找记录
 *
 * \param userdata : 用户数据指针
 * \param num_data : 数据个数
 * \param cb_comp : 比较两个数据的回调函数
 * \param data_pinpoint : 所要查找的 匹配数据指针
 * \param ret_idx : 查找到的位置;如果没有找到，则返回如添加该记录时其所在的位置。
 *
 * \return 找到则返回0，否则返回<0
 *
 * 折半方式查找记录, psl->marr 中指向的数据已经以先小后大方式排好序
 */
/**
 * \brief Using binary search to find the position by data_pin
 *
 * \param userdata : User's data
 * \param num_data : the number of items in the sorted data
 * \param cb_comp : the callback function to compare the user's data and pin
 * \param data_pinpoint : The reference data to be found
 * \param ret_idx : the position of the required data; If failed, then it is the failed position, which is the insert position if possible.
 *
 * \return 0 on found, <0 on failed(fail position is saved by ret_idx)
 *
 * Using binary search to find the position by data_pin. The user's data should be sorted.
 */
int
pf_bsearch_r (void *userdata, size_t num_data, pf_bsearch_cb_comp_t cb_comp, void *data_pinpoint, size_t *ret_idx)
{
    int retcomp;
    uint8_t flg_found;
    size_t ileft;
    size_t iright;
    size_t i;

    if (NULL == cb_comp) {
        return -1;
    }
    /* 查找合适的位置 */
    if (num_data < 1) {
        if (NULL != ret_idx) {
            *ret_idx = 0;
        }
        DBGMSG (PFDBG_CATLOG_PF, PFDBG_LEVEL_ERROR, "num_data(%"PRIuSZ") < 1", num_data);
        return -1;
    }

    /* 折半查找 */
    /* 为了不出现负数，以免缩小索引的所表示的数据范围
     * (负数表明减少一位二进制位的使用)，
     * 内部 ileft 和 iright使用从1开始的下标，
     *   即1表示C语言中的0, 2表示语言中的1，以此类推。
     * 对外还是使用以 0 为开始的下标
     */
    i = 0;
    ileft = 1;
    iright = num_data;
    flg_found = 0;
    for (; ileft <= iright;) {
        i = (ileft + iright) / 2 - 1;
        /* cb_comp should return the *userdata[i] - *data_pinpoint */
        retcomp = cb_comp (userdata, i, data_pinpoint);
        if (retcomp > 0) {
            iright = i;
        } else if (retcomp < 0) {
            ileft = i + 2;
        } else {
            /* found ! */
            flg_found = 1;
            break;
        }
    }

    if (flg_found) {
        if (NULL != ret_idx) {
            *ret_idx = i;
        }
        return 0;
    }
    if (NULL != ret_idx) {
        if (iright <= i) {
            *ret_idx = i;
        } else if (ileft >= i + 2) {
            *ret_idx = i + 1;
        }
    }
    DBGMSG (PFDBG_CATLOG_PF, PFDBG_LEVEL_DEBUG, "not found! num_data=%"PRIuSZ"; ileft=%"PRIuSZ", iright=%"PRIuSZ", i=%"PRIuSZ"", num_data, ileft, iright, i);
    return -1;
}

#if defined(CIUT_ENABLED) && (CIUT_ENABLED == 1)

#include <ciut.h>

/*"data_list[idx] - *data_pin"*/
static int
pf_bsearch_cb_comp_char(void *userdata, size_t idx, void * data_pin)
{
    char *list = (char *)userdata;
    char *data = (char *)data_pin;

    if (! data) {
        return 1;
    }

    assert (list);
    assert (data);
    if (*data == list[idx]) {
        return 0;
    } else if (*data < list[idx]) {
        return 1;
    }
    return -1;
}

TEST_CASE( .name="binsearch", .description="test search.", .skip=0 ) {
#define CSTR_LARGE "abcdefghijklmnopqrstuvwxyz"

    SECTION("test binary search callback function") {
        char pin = 'c';

        assert (sizeof(CSTR_LARGE) > 1);
        assert (sizeof(CSTR_LARGE) == 27);

        CIUT_LOG ("test the parameters %d", 1);
        REQUIRE(1 == pf_bsearch_cb_comp_char(NULL, 0, NULL));
        REQUIRE(1 == pf_bsearch_cb_comp_char(CSTR_LARGE, 0, NULL));

        pin = 'c';
        assert(pin == CSTR_LARGE[2]);
        REQUIRE(-1 == pf_bsearch_cb_comp_char(CSTR_LARGE, 0, &pin));
        REQUIRE(-1 == pf_bsearch_cb_comp_char(CSTR_LARGE, 1, &pin));
        REQUIRE(0 == pf_bsearch_cb_comp_char(CSTR_LARGE, 2, &pin));
        REQUIRE(1 == pf_bsearch_cb_comp_char(CSTR_LARGE, 3, &pin));
        REQUIRE(1 == pf_bsearch_cb_comp_char(CSTR_LARGE, sizeof(CSTR_LARGE)-2, &pin));

        pin = 'a';
        assert(pin == CSTR_LARGE[0]);
        REQUIRE(0 == pf_bsearch_cb_comp_char(CSTR_LARGE, 0, &pin));
        REQUIRE(1 == pf_bsearch_cb_comp_char(CSTR_LARGE, 1, &pin));
        REQUIRE(1 == pf_bsearch_cb_comp_char(CSTR_LARGE, 2, &pin));
        REQUIRE(1 == pf_bsearch_cb_comp_char(CSTR_LARGE, 3, &pin));
        REQUIRE(1 == pf_bsearch_cb_comp_char(CSTR_LARGE, sizeof(CSTR_LARGE)-2, &pin));

        pin = 'z';
        assert(pin == CSTR_LARGE[25]);
        REQUIRE(-1 == pf_bsearch_cb_comp_char(CSTR_LARGE, 0, &pin));
        REQUIRE(-1 == pf_bsearch_cb_comp_char(CSTR_LARGE, 1, &pin));
        REQUIRE(-1 == pf_bsearch_cb_comp_char(CSTR_LARGE, 2, &pin));
        REQUIRE(-1 == pf_bsearch_cb_comp_char(CSTR_LARGE, 3, &pin));
        REQUIRE(0 == pf_bsearch_cb_comp_char(CSTR_LARGE, sizeof(CSTR_LARGE)-2, &pin));

        pin = 'A';
        assert(pin < CSTR_LARGE[0]);
        REQUIRE(1 == pf_bsearch_cb_comp_char(CSTR_LARGE, 0, &pin));
        REQUIRE(1 == pf_bsearch_cb_comp_char(CSTR_LARGE, 1, &pin));
        REQUIRE(1 == pf_bsearch_cb_comp_char(CSTR_LARGE, 2, &pin));
        REQUIRE(1 == pf_bsearch_cb_comp_char(CSTR_LARGE, 3, &pin));
        REQUIRE(1 == pf_bsearch_cb_comp_char(CSTR_LARGE, sizeof(CSTR_LARGE)-2, &pin));

        pin = 125;
        assert(pin > CSTR_LARGE[sizeof(CSTR_LARGE)-2]);
        REQUIRE(-1 == pf_bsearch_cb_comp_char(CSTR_LARGE, 0, &pin));
        REQUIRE(-1 == pf_bsearch_cb_comp_char(CSTR_LARGE, 1, &pin));
        REQUIRE(-1 == pf_bsearch_cb_comp_char(CSTR_LARGE, 2, &pin));
        REQUIRE(-1 == pf_bsearch_cb_comp_char(CSTR_LARGE, 3, &pin));
        REQUIRE(-1 == pf_bsearch_cb_comp_char(CSTR_LARGE, sizeof(CSTR_LARGE)-2, &pin));
    }

    SECTION("test binary search") {
        size_t idx;
        char pin = 'c';

        CIUT_LOG ("test the parameters %d", 1);
        REQUIRE(-1 == pf_bsearch_r(NULL, 0, NULL, NULL, NULL));
        REQUIRE(-1 == pf_bsearch_r(NULL, 0, pf_bsearch_cb_comp_char, NULL, NULL));
        REQUIRE(-1 == pf_bsearch_r(NULL, 0, pf_bsearch_cb_comp_char, &pin, NULL));
        REQUIRE(-1 == pf_bsearch_r(NULL, 0, pf_bsearch_cb_comp_char, &pin, &idx));
        REQUIRE(-1 == pf_bsearch_r(CSTR_LARGE, sizeof(CSTR_LARGE)-1, NULL, NULL, NULL));
        REQUIRE(-1 == pf_bsearch_r(CSTR_LARGE, sizeof(CSTR_LARGE)-1, pf_bsearch_cb_comp_char, NULL, NULL));
        REQUIRE(0 == pf_bsearch_r(CSTR_LARGE, sizeof(CSTR_LARGE)-1, pf_bsearch_cb_comp_char, &pin, NULL));

        pin = 'a';
        assert(pin == CSTR_LARGE[0]);
        CIUT_LOG ("test search '%c'", pin);
        REQUIRE(0 == pf_bsearch_r(CSTR_LARGE, sizeof(CSTR_LARGE)-1, pf_bsearch_cb_comp_char, &pin, &idx));
        REQUIRE(0 == idx);

        pin = 'c';
        assert(pin == CSTR_LARGE[2]);
        CIUT_LOG ("test search '%c'", pin);
        REQUIRE(0 == pf_bsearch_r(CSTR_LARGE, sizeof(CSTR_LARGE)-1, pf_bsearch_cb_comp_char, &pin, &idx));
        //CIUT_LOG ("search(pin='%c') return idx=%d", pin, idx);
        REQUIRE(2 == idx);

        pin = 'z';
        assert(pin == CSTR_LARGE[25]);
        CIUT_LOG ("test search '%c'", pin);
        REQUIRE(0 == pf_bsearch_r(CSTR_LARGE, sizeof(CSTR_LARGE)-1, pf_bsearch_cb_comp_char, &pin, &idx));
        REQUIRE(25 == idx);

        pin = 'A';
        assert(pin < CSTR_LARGE[0]);
        idx = 1;
        CIUT_LOG ("test search '%c'", pin);
        REQUIRE(-1 == pf_bsearch_r(CSTR_LARGE, sizeof(CSTR_LARGE)-1, pf_bsearch_cb_comp_char, &pin, &idx));
        REQUIRE(0 == idx);

        pin = 125;
        assert(pin > CSTR_LARGE[sizeof(CSTR_LARGE)-2]);
        idx = 1;
        CIUT_LOG ("test search '%c'", pin);
        REQUIRE(-1 == pf_bsearch_r(CSTR_LARGE, sizeof(CSTR_LARGE)-1, pf_bsearch_cb_comp_char, &pin, &idx));
        REQUIRE(sizeof(CSTR_LARGE)-1 == idx);
    }
#undef CSTR_LARGE
}

#endif /* CIUT_ENABLED */

/**
 * \brief read the lines from file and feed the lines to user callback
 * \param fp: the FILE pointer
 * \param userdata: the user data
 * \param process: the callback function
 *
 * \return 0 on success, <0 on failed
 *
 */
int
fread_lines (FILE *fp, void * userdata, int (* process)(off_t pos, char * buf, size_t size, void *userdata))
{
    char * buffer = NULL;
    size_t szbuf = 0;
    off_t pos;
    ssize_t ret;

    szbuf = 5000;
    buffer = (char *) malloc (szbuf);
    if (NULL == buffer) {
        return -1;
    }
    pos = ftell (fp);
    // ssize_t getline (char **lineptr, size_t *n, FILE *stream)
    // char * fgets ( char * str, int num, FILE * stream );
    while ( fgets ( (char *)buffer, szbuf, fp ) != NULL ) { ret = strlen(buffer);
    //while ( (ret = getline ( &buffer, &szbuf, fp )) > 0 ) {
        process (pos, buffer, ret, userdata);
        pos = ftell (fp);
    }

    free (buffer);
    return 0;
}


/**
 * \brief read the lines from file and process the commands of the lines
 * \param fn_conf: the file name
 * \param userdata: the user data
 * \param process: the callback function
 *
 * \return N/A
 *
 */
int
read_file_lines(const char * fn_conf, void * userdata, int (* process)(off_t pos, char * buf, size_t size, void *userdata))
{
    int ret = 0;
    FILE *fp = NULL;
    if (NULL == fn_conf) {
        fp = stdin;
    } else {
        fp = fopen(fn_conf, "r");
    }
    if (NULL == fp) {
        assert (NULL != fn_conf);
        fprintf(stderr, "error in open file: '%s'.\n", fn_conf);
        return -1;
    }
    assert (NULL != fp);
    ret = 0;
    if (NULL != process) {
        ret = fread_lines (fp, userdata, process);
    }
    if (stdin != fp) {
        fclose(fp);
    }
    return ret;
}

#if defined(CIUT_ENABLED) && (CIUT_ENABLED == 1)
#include <ciut.h>

static int
process_test_readln (off_t pos, char * buf, size_t size, void *userdata)
{
    int *ret_val = userdata;
    int val;
    if (NULL != userdata) {
        val = atoi(buf);
        *ret_val += val;
    }
    return 0;
}

static int
create_test_file(const char * fn_test)
{
    FILE *fp = NULL;

    assert (NULL != fn_test);

    unlink(fn_test);

    fp = fopen(fn_test, "w+");
    if (NULL == fp) {
        fprintf(stderr, "Error in create file: '%s'\n", fn_test);
        return -1;
    }
    fprintf(fp, "1\n");
    fprintf(fp, "2\n");
    fprintf(fp, "3\n");

    fclose(fp);
    return 0;
}

TEST_CASE( .name="read-file-lines", .description="test utils.", .skip=0 ) {
#define FN_TEST "tmp-test.txt"

    //REQUIRE(0 > create_test_file("/dev/zero"));

    CIUT_LOG("Create a new file: '%s'", FN_TEST);
    REQUIRE(0 == create_test_file(FN_TEST));

    SECTION("test read file parameters") {
#if ! defined(_WIN32) // and ! MINGW
        FILE * fp_old = stdin;
        FILE * fp;
        int ret;

        fp = fopen(FN_TEST, "r");
        if (NULL != fp) {
            stdin = fp;
            ret = read_file_lines(NULL, NULL, NULL);
            fclose(fp);
            stdin = fp_old;
            REQUIRE(0 == ret);
        }

        fp = fopen(FN_TEST, "r");
        if (NULL != fp) {
            stdin = fp;
            ret = read_file_lines(NULL, NULL, process_test_readln);
            fclose(fp);
            stdin = fp_old;
            REQUIRE(0 == ret);
        }
#endif // _WIN32

#define FN_TEST2 "tmp-noexist.txt"
        unlink(FN_TEST2);
        REQUIRE(0 > read_file_lines(FN_TEST2, NULL, NULL));

        REQUIRE(0 == read_file_lines(FN_TEST, NULL, NULL));
    }
    SECTION("test read file") {
        /* create a new file */
        int val = 0;
        REQUIRE(0 == read_file_lines(FN_TEST, &val, process_test_readln));
        REQUIRE(6 == val);
    }
    unlink(FN_TEST);
}
#endif /* CIUT_ENABLED */


/**
 * \brief strip a C string and save to buffer
 * \param orig: the string to be striped
 * \param buf: the buffer to store the result
 * \param sz_buf: the size of the buffer
 *
 * \return 0 on success, <0 on failed
 *
 */
int
cstr_strip(const char * orig, char * buf, size_t sz_buf)
{
    char * pstart = (char *)orig;
    char * pend = NULL;
    size_t len;

    if (NULL == orig) {
        return -1;
    }
    len = strlen(orig);
    pend = (char *)orig + len;
    for (pstart = (char *)orig; pstart < pend; pstart ++) {
        if ((! isblank(*pstart)) && ('\n' != *pstart) && ('\r' != *pstart)) {
            break;
        }
    }
    pend --;
    for (; pstart < pend; pend --) {
        if ((! isblank(*pend)) && ('\n' != *pend) && ('\r' != *pend)) {
            break;
        }
    }
    if (NULL == buf) {
        return -1;
    }
    if (sz_buf < 1) {
        return -1;
    }
    assert (NULL != buf);
    pend ++;
    if (pstart >= pend) {
        buf[0] = 0;
        return 0;
    }
    len = pend - pstart;
    if (len > sz_buf) {
        return -2;
    }
    memmove (buf, pstart, len);
    buf[len] = 0;
    return 0;
}

#if defined(CIUT_ENABLED) && (CIUT_ENABLED == 1)

#include <ciut.h>

int
test_strip_cstr(const char *orig, const char *expected)
{
    char buf[100] = "xx";
    assert (NULL != expected);
    assert (sizeof(buf) > strlen(expected));
    if (0 > cstr_strip(orig, buf, sizeof(buf) - 1)) {
        return -1;
    }
    return strcmp(buf, expected);
}

TEST_CASE( .description="test utils.", .skip=0 ) {

    int i;

    SECTION("test cstr_strip parameters") {
        char buf[25] = "";
        //CIUT_LOG ("test the parameters");
        REQUIRE(-1 == cstr_strip(NULL, NULL, 0));
        REQUIRE(-1 == cstr_strip("", NULL, 0));
        REQUIRE(-1 == cstr_strip(NULL, buf, 0));
        REQUIRE(-1 == cstr_strip("", buf, 0));
        REQUIRE(0 == cstr_strip("", buf, sizeof(buf)));
#define CSTR_LARGE "abcdefghijklmnopqrstuvwxyz"
        assert (sizeof(CSTR_LARGE) > sizeof(buf));
        REQUIRE(-2 == cstr_strip(CSTR_LARGE, buf, sizeof(buf)));
        REQUIRE(0 == strlen(buf));

        // buffer test
#define CSTR_TEST_1_RE "abcdefghijkl"
#define CSTR_TEST_1 CSTR_TEST_1_RE " \t  \t"
        assert (sizeof(CSTR_TEST_1) < sizeof(buf));
        assert (sizeof(CSTR_TEST_1_RE) < sizeof(buf));
        strcpy(buf, CSTR_TEST_1);
        REQUIRE(0 == cstr_strip(buf, buf, sizeof(buf)));
        REQUIRE(0 == strcmp(buf, CSTR_TEST_1_RE));
#undef CSTR_TEST_1
#undef CSTR_TEST_1_RE
#define CSTR_TEST_1_RE "abcdefghijkl"
#define CSTR_TEST_1 " \t   " CSTR_TEST_1_RE
        assert (sizeof(CSTR_TEST_1) < sizeof(buf));
        assert (sizeof(CSTR_TEST_1_RE) < sizeof(buf));
        strcpy(buf, CSTR_TEST_1);
        REQUIRE(0 == cstr_strip(buf, buf, sizeof(buf)));
        REQUIRE(0 == strcmp(buf, CSTR_TEST_1_RE));
#undef CSTR_TEST_1
#undef CSTR_TEST_1_RE
#define CSTR_TEST_1_RE "abcdefghijkl"
#define CSTR_TEST_1 " \t   " CSTR_TEST_1_RE " \t  \t"
        assert (sizeof(CSTR_TEST_1) < sizeof(buf));
        assert (sizeof(CSTR_TEST_1_RE) < sizeof(buf));
        strcpy(buf, CSTR_TEST_1);
        REQUIRE(0 == cstr_strip(buf, buf, sizeof(buf)));
        REQUIRE(0 == strcmp(buf, CSTR_TEST_1_RE));
#undef CSTR_TEST_1
#undef CSTR_TEST_1_RE

#undef CSTR_LARGE
    }
    SECTION("test strip string") {
        static const char * list_strip_cstr[] = {
            "",  "", // result, orig
            "",  "\n",
            "",  "\t",
            "",  "\t\t",
            "",  "\t \t ",
            "",  " \t \t ",
            "a",  "a\n",
            "a",  "a\n\r",
            "a",  "a\n\r \n",
            "a", " a ",
            "a", "\ta\t",
            "a", "\t a \t",
            "a b", "a b",
            "a\tb", "a\tb",
            "a  b", "a  b",
            "a \t b", "a \t b",
            "", "",
        };
        for (i = 0; i < NUM_ARRAY(list_strip_cstr); i += 2) {
            CIUT_LOG ("compare strip string idx=%d", i/2);
            REQUIRE(0 == test_strip_cstr(list_strip_cstr[i + 1], list_strip_cstr[i + 0]));
        }
    }
}
#endif /* CIUT_ENABLED */

/**
 * \brief parse the long hex string and save it to a buffer
 * \param cstr: the string
 * \param cslen: the length of string
 * \param buf: the buffer
 * \param szbuf: the size of buffer
 *
 * \return >0 the length of the data in the buffer on successs, <0 on error
 *
 */
ssize_t
parse_hex_buf(char * cstr, size_t cslen, uint8_t * buf, size_t szbuf)
{
#define ER (uint8_t)(0xFF)
    static uint8_t valmap[128] = {
        ER,ER,ER,ER,ER,ER,ER,ER, ER,ER,ER,ER,ER,ER,ER,ER, ER,ER,ER,ER,ER,ER,ER,ER, ER,ER,ER,ER,ER,ER,ER,ER,
        ER,ER,ER,ER,ER,ER,ER,ER, ER,ER,ER,ER,ER,ER,ER,ER,  0, 1, 2, 3, 4, 5, 6, 7,  8, 9,ER,ER,ER,ER,ER,ER,
        ER,10,11,12,13,14,15,ER, ER,ER,ER,ER,ER,ER,ER,ER, ER,ER,ER,ER,ER,ER,ER,ER, ER,ER,ER,ER,ER,ER,ER,ER,
        ER,10,11,12,13,14,15,ER, ER,ER,ER,ER,ER,ER,ER,ER, ER,ER,ER,ER,ER,ER,ER,ER, ER,ER,ER,ER,ER,ER,ER,ER,
    };
    size_t len = 0;
    ssize_t ret = 0;
    uint8_t val;
    char *p;

    if (NULL == cstr) {
        return -1;
    }
    if (cslen == 0) {
        return 0;
    }
    if (cslen < 3) {
        return -1;
    }
    assert (strlen(cstr) == cslen);
    assert (2 < cslen);
    if (0 != strncmp("0x", cstr, 2)) {
        return -1;
    }
    // skip first '0x'
    p = cstr + 2;
    while (len + 2 < cslen && ret < szbuf) {
        assert (*p < sizeof(valmap));
        val = valmap[(int)(*p)];
        if (val == ER) {
            return ret;
        }
        len ++;
        if (len % 2 == 0) {
            buf[ret] += val;
            ret ++;
        } else {
            buf[ret] = val << 4;
        }
        p ++;
    }
    return ret;
#undef ER
}

#if defined(CIUT_ENABLED) && (CIUT_ENABLED == 1)
#include <ciut.h>

int
test_parse_hex_buf(char * static_string, size_t len)
{
    ssize_t ret;
    ssize_t i;
    uint8_t buffer1[100] = "11223344556677889900";
    uint8_t buffer2[100] = "aabbccddeeffgghhiijj";
    uint8_t *p;

    assert (sizeof(buffer1) > len);
    assert (sizeof(buffer2) > len);
    memset(buffer1, 0, sizeof(buffer1));
    ret = parse_hex_buf(static_string, len, buffer1, sizeof(buffer1));
    fprintf(stderr, "dump of parse_hex_buf, size=%" PRIiSZ ":\n", ret);
    hex_dump_to_fd(STDERR_FILENO, (opaque_t *)(buffer1), ret);

    memset(buffer2, 0, sizeof(buffer2));
    strcpy((char *)buffer2, "0x");
    p = buffer2 + 2;
    for (i = 0; i < ret; i ++) {
        sprintf((char *)p, "%02x", buffer1[i]);
        p = p + 2;
    }
    // to lower case for string
    memset(buffer1, 0, sizeof(buffer1));
    for (i = 0; i < len; i ++) {
        buffer1[i] = tolower(static_string[i]);
    }
    if (0 == strcmp((char *)buffer1, (char *)buffer2)) {
        return 0;
    }

    return -1;
}

TEST_CASE( .description="test hex string.", .skip=0 ) {

    SECTION("test hex string parameters") {
        uint8_t buf[] = "11223344556677889900";
        REQUIRE (0 > parse_hex_buf(NULL, 0, NULL, 0));
        REQUIRE (0 > parse_hex_buf(NULL, 0, (uint8_t *)"", 0));
        REQUIRE (0 == parse_hex_buf("", 0, (uint8_t *)"", 0));
        REQUIRE (0 > parse_hex_buf("a", 1, (uint8_t *)"", 0));
        REQUIRE (0 > parse_hex_buf("ab", 2, (uint8_t *)"", 0));
        REQUIRE (0 > parse_hex_buf("a", 1, buf, sizeof(buf) - 1));
        REQUIRE (0 > parse_hex_buf("ab", 2, buf, sizeof(buf) - 1));
        REQUIRE (0 > parse_hex_buf("0x", 2, buf, sizeof(buf) - 1));
        REQUIRE (0 == parse_hex_buf("0x0", 3, buf, sizeof(buf) - 1));
        REQUIRE (0 > parse_hex_buf("ab0", 3, buf, sizeof(buf) - 1));
    }
    SECTION("test hex string") {
        //CIUT_LOG ("test the hex");
#define TEST_2HEX(static_string) REQUIRE (0 == test_parse_hex_buf(static_string, sizeof(static_string)-1))
        TEST_2HEX("0x313924746201");
        TEST_2HEX("0x983923492313442837987510983740");
        TEST_2HEX("0xe932be9df8");
        TEST_2HEX("0xeD3Ebe9Af8");
#undef TEST_2HEX
    }
}

#endif /* CIUT_ENABLED */
