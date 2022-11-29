# **Lab 2: Lexical Analysis Doc**

520021910607 谈子铭

## comments

遇到表明注释开始的符号 /* 之后转入 StartCondition::COMMENT，并设置初始注释层级深度为1；之后在 COMMENT 初始条件下，再遇到 /* 就增加注释层深度，遇到 */ 则检查当前注释层深度是否为1（是表明注释结束，转到 StartCondition::INITIAL，否则将注释层深度减一）；遇到其余符号均直接忽略，包括换行符

## strings

遇到表明字符串开始的符号 “ 之后转入 StartCondition::STRING；之后在 STR 初始条件下，遇到 ” 表明字符串结束，将整个读入的字符串（位于 string_buf_ 中）认为是所匹配的内容；值得注意的是对于特殊符号的处理。遇到 “\n” “\t” 等时需要将其转换为真实的意义（换行，空格）；遇到控制字符（\^C等）需要转换为其 asc 码；遇到 \ddd 需要转换为三位数对应的 char；遇到 string 内的 “  \ 等（用转义标注）需要将对应字符添加到 string_buf_ 中；遇到 \ 与 \ 间的各种空白字符应当直接忽略。对于其他字符，直接加入到 string_buf_ 中。

## error handling

当遇到非法输入时，直接报错 errormsg_->Error(errormsg_->tok_pos_, "illegal token")

## end-of-file handling

最后处理 <\<EOF\>>，遇到之后直接正常结束即可