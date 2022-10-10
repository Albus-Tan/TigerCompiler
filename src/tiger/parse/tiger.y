%filenames parser
%scanner tiger/lex/scanner.h
%baseclass-preinclude tiger/absyn/absyn.h

 /*
  * Please don't modify the lines above.
  */

%union {
  int ival;
  std::string* sval;
  sym::Symbol *sym;
  absyn::Exp *exp;
  absyn::ExpList *explist;
  absyn::Var *var;
  absyn::DecList *declist;
  absyn::Dec *dec;
  absyn::EFieldList *efieldlist;
  absyn::EField *efield;
  absyn::NameAndTyList *tydeclist;
  absyn::NameAndTy *tydec;
  absyn::FieldList *fieldlist;
  absyn::Field *field;
  absyn::FunDecList *fundeclist;
  absyn::FunDec *fundec;
  absyn::Ty *ty;
  }

%token <sym> ID
%token <sval> STRING
%token <ival> INT

%token
  COMMA COLON SEMICOLON LPAREN RPAREN LBRACK RBRACK
  LBRACE RBRACE DOT
  ASSIGN
  ARRAY IF THEN ELSE WHILE FOR TO DO LET IN END OF
  BREAK NIL
  FUNCTION VAR TYPE

 /* token priority */
 /* TODO: Put your lab3 code here */
 /* bottom ones have higher priority */
%left OR
%left AND
%nonassoc EQ NEQ LT LE GT GE
%left PLUS MINUS
%left TIMES DIVIDE
%right UMINUS

%type <exp> exp expseq
%type <explist> actuals nonemptyactuals sequencing sequencing_exps
%type <var> lvalue one oneormore
%type <declist> decs decs_nonempty
%type <dec> decs_nonempty_s vardec
%type <efieldlist> rec rec_nonempty
%type <efield> rec_one
%type <tydeclist> tydec
%type <tydec> tydec_one
%type <fieldlist> tyfields tyfields_nonempty
%type <field> tyfield
%type <ty> ty
%type <fundeclist> fundec
%type <fundec> fundec_one

%start program

%%
program:  exp  {absyn_tree_ = std::make_unique<absyn::AbsynTree>($1);};

 /* TODO: Put your lab3 code here */

/* Left Value */
lvalue:  lvalue DOT ID  {$$ = new absyn::FieldVar(scanner_.GetTokPos(), $1, $3);}
  |  ID LBRACK exp RBRACK  {$$ = new absyn::SubscriptVar(scanner_.GetTokPos(), new absyn::SimpleVar(scanner_.GetTokPos(), $1), $3);}
  |  lvalue LBRACK exp RBRACK  {$$ = new absyn::SubscriptVar(scanner_.GetTokPos(), $1, $3);}
  |  ID  {$$ = new absyn::SimpleVar(scanner_.GetTokPos(), $1);}
  ;

/* Expressions */

/* VarExp */
/* left value */
exp:  lvalue  {$$ = new absyn::VarExp(scanner_.GetTokPos(), $1);};

/* NilExp */
exp:  NIL  {$$ = new absyn::NilExp(scanner_.GetTokPos());};

/* IntExp */
exp:  INT  {$$ = new absyn::IntExp(scanner_.GetTokPos(), $1);};

/* StringExp */
exp:  STRING  {$$ = new absyn::StringExp(scanner_.GetTokPos(), $1);};

/* CallExp */
/* id() */
exp :  ID LPAREN RPAREN {$$ = new absyn::CallExp(scanner_.GetTokPos(), $1, new absyn::ExpList());};
/* id(exp{,exp}) */
exp :  ID LPAREN nonemptyactuals RPAREN {$$ = new absyn::CallExp(scanner_.GetTokPos(), $1, $3);};

/* OpExp */
exp :  exp AND exp  {$$ = new absyn::OpExp(scanner_.GetTokPos(), absyn::AND_OP, $1, $3);};
exp :  exp OR exp  {$$ = new absyn::OpExp(scanner_.GetTokPos(), absyn::OR_OP, $1, $3);};
exp :  exp PLUS exp  {$$ = new absyn::OpExp(scanner_.GetTokPos(), absyn::PLUS_OP, $1, $3);};
exp :  exp MINUS exp  {$$ = new absyn::OpExp(scanner_.GetTokPos(), absyn::MINUS_OP, $1, $3);};
exp :  exp TIMES exp  {$$ = new absyn::OpExp(scanner_.GetTokPos(), absyn::TIMES_OP, $1, $3);};
exp :  exp DIVIDE exp  {$$ = new absyn::OpExp(scanner_.GetTokPos(), absyn::DIVIDE_OP, $1, $3);};
exp :  exp EQ exp  {$$ = new absyn::OpExp(scanner_.GetTokPos(), absyn::EQ_OP, $1, $3);};
exp :  exp NEQ exp  {$$ = new absyn::OpExp(scanner_.GetTokPos(), absyn::NEQ_OP, $1, $3);};
exp :  exp LT exp  {$$ = new absyn::OpExp(scanner_.GetTokPos(), absyn::LT_OP, $1, $3);};
exp :  exp LE exp  {$$ = new absyn::OpExp(scanner_.GetTokPos(), absyn::LE_OP, $1, $3);};
exp :  exp GT exp  {$$ = new absyn::OpExp(scanner_.GetTokPos(), absyn::GT_OP, $1, $3);};
exp :  exp GE exp  {$$ = new absyn::OpExp(scanner_.GetTokPos(), absyn::GE_OP, $1, $3);};
/* use 0 - INT to represent exp -INT */
exp :  MINUS exp %prec UMINUS {$$ = new absyn::OpExp(scanner_.GetTokPos(), absyn::MINUS_OP, new absyn::IntExp(scanner_.GetTokPos(), 0), $2);};

/* RecordExp */
/* type-id{} */
/* type-id{id=exp{,id=exp}} */
exp :  ID LBRACE rec RBRACE {$$ = new absyn::RecordExp(scanner_.GetTokPos(), $1, $3);};

/* SeqExp */
/* (exp;exp;...exp) */
exp :  LPAREN expseq RPAREN {$$ = $2;};

/* AssignExp */
exp :  lvalue ASSIGN exp {$$ = new absyn::AssignExp(scanner_.GetTokPos(), $1, $3);};

/* IfExp */
exp :  IF exp THEN exp ELSE exp  {$$ = new absyn::IfExp(scanner_.GetTokPos(), $2, $4, $6);};
exp :  IF exp THEN exp  {$$ = new absyn::IfExp(scanner_.GetTokPos(), $2, $4, nullptr);};

/* WhileExp */
exp :  WHILE exp DO exp {$$ = new absyn::WhileExp(scanner_.GetTokPos(), $2, $4);};

/* ForExp */
exp :  FOR ID ASSIGN exp TO exp DO exp {$$ = new absyn::ForExp(scanner_.GetTokPos(), $2, $4, $6, $8);};

/* BreakExp */
exp :  BREAK  {$$ = new absyn::BreakExp(scanner_.GetTokPos());};

/* LetExp */
exp :  LET decs IN expseq END {$$ = new absyn::LetExp(scanner_.GetTokPos(), $2, $4);};

/* ArrayExp */
/* type-id[exp] of exp */
exp :  ID LBRACK exp RBRACK OF exp {$$ = new absyn::ArrayExp(scanner_.GetTokPos(), $1, $3, $6);};

/* VoidExp */
exp :  LPAREN RPAREN {$$ = new absyn::VoidExp(scanner_.GetTokPos());};

/* Wrapped by paren */
exp :  LPAREN exp RPAREN {$$ = $2;};

/* SeqExp */
/* expseq */
/* exp;exp;...exp */
expseq :  sequencing_exps {$$ = new absyn::SeqExp(scanner_.GetTokPos(), $1);};

/* sequencing_exps */
/* exp */
sequencing_exps :  exp {$$ = new absyn::ExpList($1);};
/* exp;exp;...exp */
sequencing_exps :  exp SEMICOLON sequencing_exps {$$ = $3; $$->Prepend($1);};

/* nonemptyactuals */
/* exp */
nonemptyactuals :  exp {$$ = new absyn::ExpList($1);};
/* exp{,exp} */
nonemptyactuals :  exp COMMA nonemptyactuals {$$ = $3; $$->Prepend($1);};

/* Declaration */
/* decs */
decs :  decs_nonempty {$$ = $1;}
  |
  ;
/* decs_nonempty */
decs_nonempty :  decs_nonempty_s {$$ = new absyn::DecList($1);}
  |  decs_nonempty_s decs_nonempty {$$ = $2; $$->Prepend($1);}
  ;
/* dec */
decs_nonempty_s :  tydec {$$ = new absyn::TypeDec(scanner_.GetTokPos(), $1);}
  |  vardec {$$ = $1;}
  |  fundec {$$ = new absyn::FunctionDec(scanner_.GetTokPos(), $1);}
  ;
/* tydec */
tydec :  tydec_one {$$ = new absyn::NameAndTyList($1);}
  |  tydec_one tydec {$$ = $2; $$->Prepend($1);}
  ;
/* type type-id = ty */
tydec_one :  TYPE ID EQ ty {$$ = new absyn::NameAndTy($2, $4);};


/* vardec */
vardec :  VAR ID ASSIGN exp {$$ = new absyn::VarDec(scanner_.GetTokPos(), $2, nullptr, $4);}
  |  VAR ID COLON ID ASSIGN exp {$$ = new absyn::VarDec(scanner_.GetTokPos(), $2, $4, $6);}
  ;

/* fundec */
fundec :  fundec_one {$$ = new absyn::FunDecList($1);}
  |  fundec_one fundec {$$ = $2; $$->Prepend($1);}
  ;
fundec_one :  FUNCTION ID LPAREN tyfields RPAREN EQ exp {$$ = new absyn::FunDec(scanner_.GetTokPos(), $2, $4, nullptr, $7);}
  |  FUNCTION ID LPAREN tyfields RPAREN COLON ID EQ exp {$$ = new absyn::FunDec(scanner_.GetTokPos(), $2, $4, $7, $9);}
  ;

/* tyfields */
tyfields :  tyfields_nonempty {$$ = $1;}
  |  {$$ = new absyn::FieldList();}
  ;
/* id : type-id{, id : type-id} */
tyfields_nonempty :  tyfield {$$ = new absyn::FieldList($1);}
  |  tyfield COMMA tyfields_nonempty {$$ = $3; $$->Prepend($1);}
  ;
/* id : type-id */
tyfield :  ID COLON ID {$$ = new absyn::Field(scanner_.GetTokPos(), $1, $3);};

/* Types */
/* ty */
ty :  ID {$$ = new absyn::NameTy(scanner_.GetTokPos(), $1);}
  |  LBRACE tyfields RBRACE {$$ = new absyn::RecordTy(scanner_.GetTokPos(), $2);}
  |  ARRAY OF ID {$$ = new absyn::ArrayTy(scanner_.GetTokPos(), $3);}
  ;

/* rec */
rec :  rec_nonempty {$$ = $1;}
  |  {$$ = new absyn::EFieldList();}
  ;
rec_nonempty :  rec_one {$$ = new absyn::EFieldList($1);}
  |  rec_one COMMA rec_nonempty {$$ = $3; $$->Prepend($1);}
  ;
rec_one :  ID EQ exp {$$ = new absyn::EField($1, $3);};