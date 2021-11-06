struct expression;
struct toml : tao::pegtl::seq<expression,
                              tao::pegtl::star<tao::pegtl::eol, expression>> {};
struct ws : tao::pegtl::star<tao::pegtl::blank> {};
struct comment;
struct keyval;
struct table;
struct expression
    : tao::pegtl::sor<
          tao::pegtl::seq<ws, tao::pegtl::opt<comment>>,
          tao::pegtl::seq<ws, keyval, ws, tao::pegtl::opt<comment>>,
          tao::pegtl::seq<ws, table, ws, tao::pegtl::opt<comment>>> {};
struct comment_start_symbol : tao::pegtl::one<'#'> {};
struct non_ascii : tao::pegtl::sor<tao::pegtl::utf32::range<0x80, 0xD7FF>,
                                   tao::pegtl::utf32::range<0xE000, 0x10FFFF>> {
};
struct non_eol : tao::pegtl::sor<tao::pegtl::one<0x09>,
                                 tao::pegtl::range<0x20, 0x7E>, non_ascii> {};
struct comment
    : tao::pegtl::seq<comment_start_symbol, tao::pegtl::star<non_eol>> {};
struct key;
struct keyval_sep : tao::pegtl::pad<tao::pegtl::one<'='>, tao::pegtl::blank> {};
struct val;
struct keyval : tao::pegtl::seq<key, keyval_sep, val> {};
struct simple_key;
struct dotted_key;
struct key : tao::pegtl::sor<simple_key, dotted_key> {};
struct quoted_key;
struct unquoted_key;
struct simple_key : tao::pegtl::sor<quoted_key, unquoted_key> {};
struct unquoted_key
    : tao::pegtl::plus<tao::pegtl::sor<tao::pegtl::alpha, tao::pegtl::digit,
                                       tao::pegtl::one<'-', '_'>>> {};
struct basic_string;
struct literal_string;
struct quoted_key : tao::pegtl::sor<basic_string, literal_string> {};
struct dot_sep : tao::pegtl::pad<tao::pegtl::one<'.'>, tao::pegtl::blank> {};
struct dotted_key
    : tao::pegtl::seq<simple_key, tao::pegtl::plus<dot_sep, simple_key>> {};
struct string;
struct TRUE : tao::pegtl::string<0x74, 0x72, 0x75, 0x65> {};
struct FALSE : tao::pegtl::string<0x66, 0x61, 0x6C, 0x73, 0x65> {};
struct boolean : tao::pegtl::sor<TRUE, FALSE> {};
struct array;
struct inline_table;
struct date_time;
struct FLOAT;
struct unsigned_dec_int;
struct minus : tao::pegtl::one<'-'> {};
struct plus : tao::pegtl::one<'+'> {};
struct digit1_9 : tao::pegtl::range<'1', '9'> {};
struct digit0_1 : tao::pegtl::range<'0', '1'> {};
struct hex_prefix : tao::pegtl::string<0x30, 0x78> {};
struct oct_prefix : tao::pegtl::string<0x30, 0x6F> {};
struct bin_prefix : tao::pegtl::string<0x30, 0x62> {};
struct underscore : tao::pegtl::one<0x5F> {};
struct dec_int : tao::pegtl::seq<tao::pegtl::opt<tao::pegtl::sor<minus, plus>>,
                                 unsigned_dec_int> {};
struct HEXDIG
    : tao::pegtl::sor<tao::pegtl::digit, tao::pegtl::istring<'A'>,
                      tao::pegtl::istring<'B'>, tao::pegtl::istring<'C'>,
                      tao::pegtl::istring<'D'>, tao::pegtl::istring<'E'>,
                      tao::pegtl::istring<'F'>> {};
struct hex_int
    : tao::pegtl::seq<hex_prefix, HEXDIG,
                      tao::pegtl::star<tao::pegtl::sor<
                          HEXDIG, tao::pegtl::seq<underscore, HEXDIG>>>> {};
struct oct_int
    : tao::pegtl::seq<oct_prefix, tao::pegtl::odigit,
                      tao::pegtl::star<tao::pegtl::sor<
                          tao::pegtl::odigit,
                          tao::pegtl::seq<underscore, tao::pegtl::odigit>>>> {};
struct bin_int
    : tao::pegtl::seq<bin_prefix, digit0_1,
                      tao::pegtl::star<tao::pegtl::sor<
                          digit0_1, tao::pegtl::seq<underscore, digit0_1>>>> {};
struct integer : tao::pegtl::sor<dec_int, hex_int, oct_int, bin_int> {};
struct val : tao::pegtl::sor<string, boolean, array, inline_table, date_time,
                             FLOAT, integer> {};
struct ml_basic_string;
struct ml_literal_string;
struct string : tao::pegtl::sor<ml_basic_string, basic_string,
                                ml_literal_string, literal_string> {};
struct quotation_mark;
struct basic_char;
struct basic_string
    : tao::pegtl::seq<quotation_mark, tao::pegtl::star<basic_char>,
                      quotation_mark> {};
struct quotation_mark : tao::pegtl::one<0x22> {};
struct basic_unescaped;
struct escaped;
struct basic_char : tao::pegtl::sor<basic_unescaped, escaped> {};
struct basic_unescaped
    : tao::pegtl::sor<tao::pegtl::blank, tao::pegtl::one<0x21>,
                      tao::pegtl::range<0x23, 0x5B>,
                      tao::pegtl::range<0x5D, 0x7E>, non_ascii> {};
struct escape : tao::pegtl::one<0x5C> {};
struct escape_seq_char;
struct escaped : tao::pegtl::seq<escape, escape_seq_char> {};
struct escape_seq_char
    : tao::pegtl::sor<
          tao::pegtl::one<0x22>, tao::pegtl::one<0x5C>, tao::pegtl::one<0x62>,
          tao::pegtl::one<0x66>, tao::pegtl::one<0x6E>, tao::pegtl::one<0x72>,
          tao::pegtl::one<0x74>,
          tao::pegtl::seq<tao::pegtl::one<0x75>, tao::pegtl::rep<4, HEXDIG>>,
          tao::pegtl::seq<tao::pegtl::one<0x55>, tao::pegtl::rep<8, HEXDIG>>> {
};
struct ml_basic_string_delim;
struct ml_basic_body;
struct ml_basic_string
    : tao::pegtl::seq<ml_basic_string_delim, tao::pegtl::opt<tao::pegtl::eol>,
                      ml_basic_body, ml_basic_string_delim> {};
struct ml_basic_string_delim : tao::pegtl::rep<3, quotation_mark> {};
struct mlb_content;
struct mlb_quotes;
struct ml_basic_body
    : tao::pegtl::seq<
          tao::pegtl::star<mlb_content>,
          tao::pegtl::star<mlb_quotes, tao::pegtl::plus<mlb_content>>,
          tao::pegtl::opt<mlb_quotes>> {};
struct mlb_char;
struct mlb_escaped_nl;
struct mlb_content
    : tao::pegtl::sor<mlb_char, tao::pegtl::eol, mlb_escaped_nl> {};
struct mlb_unescaped;
struct mlb_char : tao::pegtl::sor<mlb_unescaped, escaped> {};
struct mlb_quotes
    : tao::pegtl::seq<quotation_mark, tao::pegtl::opt<quotation_mark>> {};
struct mlb_unescaped
    : tao::pegtl::sor<tao::pegtl::blank, tao::pegtl::one<0x21>,
                      tao::pegtl::range<0x23, 0x5B>,
                      tao::pegtl::range<0x5D, 0x7E>, non_ascii> {};
struct mlb_escaped_nl
    : tao::pegtl::seq<escape, ws, tao::pegtl::eol,
                      tao::pegtl::star<tao::pegtl::sor<tao::pegtl::blank,
                                                       tao::pegtl::eol>>> {};
struct apostrophe : tao::pegtl::one<0x27> {};
struct literal_char;
struct literal_string
    : tao::pegtl::seq<apostrophe, tao::pegtl::star<literal_char>, apostrophe> {
};
struct literal_char
    : tao::pegtl::sor<tao::pegtl::one<0x09>, tao::pegtl::range<0x20, 0x26>,
                      tao::pegtl::range<0x28, 0x7E>, non_ascii> {};
struct ml_literal_string_delim;
struct ml_literal_body;
struct ml_literal_string
    : tao::pegtl::seq<ml_literal_string_delim, tao::pegtl::opt<tao::pegtl::eol>,
                      ml_literal_body, ml_literal_string_delim> {};
struct ml_literal_string_delim : tao::pegtl::rep<3, apostrophe> {};
struct mll_content;
struct mll_quotes;
struct ml_literal_body
    : tao::pegtl::seq<
          tao::pegtl::star<mll_content>,
          tao::pegtl::star<mll_quotes, tao::pegtl::plus<mll_content>>,
          tao::pegtl::opt<mll_quotes>> {};
struct mll_char;
struct mll_content : tao::pegtl::sor<mll_char, tao::pegtl::eol> {};
struct mll_char
    : tao::pegtl::sor<tao::pegtl::one<0x09>, tao::pegtl::range<0x20, 0x26>,
                      tao::pegtl::range<0x28, 0x7E>, non_ascii> {};
struct mll_quotes : tao::pegtl::seq<apostrophe, tao::pegtl::opt<apostrophe>> {};
struct unsigned_dec_int
    : tao::pegtl::sor<
          tao::pegtl::digit,
          tao::pegtl::seq<
              digit1_9, tao::pegtl::plus<tao::pegtl::sor<
                            tao::pegtl::digit,
                            tao::pegtl::seq<underscore, tao::pegtl::digit>>>>> {
};
struct float_int_part;
struct exp;
struct frac;
struct inf : tao::pegtl::string<0x69, 0x6e, 0x66> {};
struct nan : tao::pegtl::string<0x6e, 0x61, 0x6e> {};
struct special_float
    : tao::pegtl::seq<tao::pegtl::opt<tao::pegtl::sor<minus, plus>>,
                      tao::pegtl::sor<inf, nan>> {};

struct FLOAT
    : tao::pegtl::sor<
          tao::pegtl::seq<
              float_int_part,
              tao::pegtl::sor<exp,
                              tao::pegtl::seq<frac, tao::pegtl::opt<exp>>>>,
          special_float> {};
struct float_int_part : dec_int {};
struct decimal_point;
struct zero_prefixable_int;
struct frac : tao::pegtl::seq<decimal_point, zero_prefixable_int> {};
struct decimal_point : tao::pegtl::one<0x2E> {};
struct zero_prefixable_int
    : tao::pegtl::seq<tao::pegtl::digit,
                      tao::pegtl::star<tao::pegtl::sor<
                          tao::pegtl::digit,
                          tao::pegtl::seq<underscore, tao::pegtl::digit>>>> {};
struct float_exp_part;
struct exp : tao::pegtl::seq<tao::pegtl::istring<'e'>, float_exp_part> {};
struct float_exp_part
    : tao::pegtl::seq<tao::pegtl::opt<tao::pegtl::sor<minus, plus>>,
                      zero_prefixable_int> {};
struct offset_date_time;
struct local_date_time;
struct local_date;
struct local_time;
struct date_time : tao::pegtl::sor<offset_date_time, local_date_time,
                                   local_date, local_time> {};
struct date_fullyear : tao::pegtl::rep<4, tao::pegtl::digit> {};
struct date_month : tao::pegtl::rep<2, tao::pegtl::digit> {};
struct date_mday : tao::pegtl::rep<2, tao::pegtl::digit> {};
struct time_delim
    : tao::pegtl::sor<tao::pegtl::istring<'T'>, tao::pegtl::one<0x20>> {};
struct time_hour : tao::pegtl::rep<2, tao::pegtl::digit> {};
struct time_minute : tao::pegtl::rep<2, tao::pegtl::digit> {};
struct time_second : tao::pegtl::rep<2, tao::pegtl::digit> {};
struct time_secfrac : tao::pegtl::seq<tao::pegtl::one<'.'>,
                                      tao::pegtl::plus<tao::pegtl::digit>> {};
struct time_numoffset
    : tao::pegtl::seq<
          tao::pegtl::sor<tao::pegtl::one<'+'>, tao::pegtl::one<'-'>>,
          time_hour, tao::pegtl::one<':'>, time_minute> {};
struct time_offset : tao::pegtl::sor<tao::pegtl::istring<'Z'>, time_numoffset> {
};
struct partial_time
    : tao::pegtl::seq<time_hour, tao::pegtl::one<':'>, time_minute,
                      tao::pegtl::one<':'>, time_second,
                      tao::pegtl::opt<time_secfrac>> {};
struct full_date
    : tao::pegtl::seq<date_fullyear, tao::pegtl::one<'-'>, date_month,
                      tao::pegtl::one<'-'>, date_mday> {};
struct full_time : tao::pegtl::seq<partial_time, time_offset> {};
struct offset_date_time : tao::pegtl::seq<full_date, time_delim, full_time> {};
struct local_date_time : tao::pegtl::seq<full_date, time_delim, partial_time> {
};
struct local_date : full_date {};
struct local_time : partial_time {};
struct array_open : tao::pegtl::one<'['> {};
struct array_close : tao::pegtl::one<']'> {};
struct array_sep : tao::pegtl::one<','> {};
struct array_values;
struct ws_comment_newline;
struct array : tao::pegtl::seq<array_open, tao::pegtl::opt<array_values>,
                               ws_comment_newline, array_close> {};
struct array_values
    : tao::pegtl::sor<
          tao::pegtl::seq<ws_comment_newline, val, ws_comment_newline,
                          array_sep, array_values>,
          tao::pegtl::seq<ws_comment_newline, val, ws_comment_newline,
                          tao::pegtl::opt<array_sep>>> {};
struct ws_comment_newline
    : tao::pegtl::star<tao::pegtl::sor<
          tao::pegtl::blank,
          tao::pegtl::seq<tao::pegtl::opt<comment>, tao::pegtl::eol>>> {};
struct std_table;
struct array_table;
struct table : tao::pegtl::sor<std_table, array_table> {};
struct std_table_open;
struct std_table_close;
struct std_table : tao::pegtl::seq<std_table_open, key, std_table_close> {};
struct std_table_open : tao::pegtl::seq<tao::pegtl::one<0x5B>, ws> {};
struct std_table_close : tao::pegtl::seq<ws, tao::pegtl::one<0x5D>> {};
struct inline_table_open;
struct inline_table_keyvals;
struct inline_table_close;
struct inline_table
    : tao::pegtl::seq<inline_table_open, tao::pegtl::opt<inline_table_keyvals>,
                      inline_table_close> {};
struct inline_table_open : tao::pegtl::seq<tao::pegtl::one<0x7B>, ws> {};
struct inline_table_close : tao::pegtl::seq<ws, tao::pegtl::one<0x7D>> {};
struct inline_table_sep : tao::pegtl::seq<ws, tao::pegtl::one<0x2C>, ws> {};
struct inline_table_keyvals
    : tao::pegtl::seq<keyval, tao::pegtl::opt<tao::pegtl::seq<
                                  inline_table_sep, inline_table_keyvals>>> {};
struct array_table_open;
struct array_table_close;
struct array_table : tao::pegtl::seq<array_table_open, key, array_table_close> {
};
struct array_table_open : tao::pegtl::seq<tao::pegtl::string<0x5B, 0x5B>, ws> {
};
struct array_table_close : tao::pegtl::seq<ws, tao::pegtl::string<0x5D, 0x5D>> {
};
