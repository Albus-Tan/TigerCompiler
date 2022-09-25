%filenames = "scanner"

 /*
  * Please don't modify the lines above.
  */

 /* You can add lex definitions here. */
 /* TODO: Put your lab2 code here */

%x COMMENT STR IGNORE

%%

 /*
  * Below is examples, which you can wipe out
  * and write regular expressions and actions of your own.
  */

 /* reserved words */
"array" {adjust(); return Parser::ARRAY;}
"if" {adjust(); return Parser::IF;}
"then" {adjust(); return Parser::THEN;}
"else" {adjust(); return Parser::ELSE;}
"while" {adjust(); return Parser::WHILE;}
"for" {adjust(); return Parser::FOR;}
"to" {adjust(); return Parser::TO;}
"do" {adjust(); return Parser::DO;}
"let" {adjust(); return Parser::LET;}
"in" {adjust(); return Parser::IN;}
"end" {adjust(); return Parser::END;}
"of" {adjust(); return Parser::OF;}
"break" {adjust(); return Parser::BREAK;}
"nil" {adjust(); return Parser::NIL;}
"function" {adjust(); return Parser::FUNCTION;}
"var" {adjust(); return Parser::VAR;}
"type" {adjust(); return Parser::TYPE;}

/* special characters */
"," {adjust(); return Parser::COMMA;}
":" {adjust(); return Parser::COLON;}
";" {adjust(); return Parser::SEMICOLON;}
"(" {adjust(); return Parser::LPAREN;}
")" {adjust(); return Parser::RPAREN;}
"[" {adjust(); return Parser::LBRACK;}
"]" {adjust(); return Parser::RBRACK;}
"{" {adjust(); return Parser::LBRACE;}
"}" {adjust(); return Parser::RBRACE;}
"." {adjust(); return Parser::DOT;}
"+" {adjust(); return Parser::PLUS;}
"-" {adjust(); return Parser::MINUS;}
"*" {adjust(); return Parser::TIMES;}
"/" {adjust(); return Parser::DIVIDE;}
"=" {adjust(); return Parser::EQ;}
"<>" {adjust(); return Parser::NEQ;}
"<" {adjust(); return Parser::LT;}
"<=" {adjust(); return Parser::LE;}
">" {adjust(); return Parser::GT;}
">=" {adjust(); return Parser::GE;}
"&" {adjust(); return Parser::AND;}
"|" {adjust(); return Parser::OR;}
":=" {adjust(); return Parser::ASSIGN;}

 /* identifiers */
[a-zA-Z][a-zA-Z0-9_]* {adjust(); string_buf_ = matched(); return Parser::ID;}

 /* integer */
[0-9]+ {adjust(); string_buf_ = matched(); return Parser::INT;}

 /* string */
 \" {adjust(); begin(StartCondition__::STR); string_buf_.clear(); }

 <STR>{
    \" {adjustStr(); setMatched(string_buf_); begin(StartCondition__::INITIAL); return Parser::STRING;}
    \\n {adjustStr(); string_buf_ += "\n";}
    \\t {adjustStr(); string_buf_ += "\t";}

    /* \^c */
    "\\^"[A-Z] {adjustStr(); string_buf_ += char(matched()[2] - 'A' + 1);}

    /* \ddd ignore \ , change three digits int ddd to char */
    \\[0-9]{3} {adjustStr(); string_buf_ += (char)atoi(1 + matched().c_str());}

    /* deal with \" in string */
    \\\" {adjustStr(); string_buf_ += '\"';}

    /* deal with \\ in string */
    \\\\ {adjustStr(); string_buf_ += '\\';}

    /* white space between \ and \ */
    \\[ \t\n\f]+\\ {adjustStr();}

    . {adjustStr(); string_buf_ += matched();}
 }

 /* comment */
 "/*" {adjust(); begin(StartCondition__::COMMENT); comment_level_ = 1;}
 <COMMENT>{
    "*/" {adjustStr(); if(comment_level_ == 1) begin(StartCondition__::INITIAL); else --comment_level_;}
    "/*" {adjustStr(); ++comment_level_;}
    \n {adjustStr();}
    . {adjustStr();}
 }

 /*
  * skip white space chars.
  * space, tabs and LF
  */
[ \t]+ {adjust();}
\n {adjust(); errormsg_->Newline();}

 /* illegal input */
. {adjust(); errormsg_->Error(errormsg_->tok_pos_, "illegal token");}
