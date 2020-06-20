# オレオレ C コンパイラー

コンパイラのお勉強用。

参考
[低レイヤを知りたい人のための C コンパイラ作成入門](https://www.sigbus.info/compilerbook#%E6%95%B4%E6%95%B0%E3%83%AC%E3%82%B8%E3%82%B9%E3%82%BF%E3%81%AE%E4%B8%80%E8%A6%A7)

<!-- # 生成規則 -->

<!-- マクロ -->
<!-- ``` -->
<!-- macro = "#" define -->

<!-- define = "define" ( ident ( num | string-literal )? ) -->
<!--        | ( ident "(" ident ( "," ident )* ")" " " "(" expression ")" ) -->
<!-- ``` -->

<!-- 関数とか変数の宣言から式全般 -->
<!-- ``` -->
<!-- program = ( funcdef | declaration ";" )* -->
<!-- typespec = "int" | "char" | struct-declarator | enum-declarator -->
<!-- typename = typespec pointers -->
<!-- pointers = ( "*" )* -->
<!-- funcdef = typespec declarator compound-stmt -->

<!-- declarator = pointers ident type-suffix -->
<!-- struct-declarator = "struct" ident "{" sturct-member "}" ";" -->
<!-- struct-member = (typespec declarator ( "," declarator )* ";" )* -->
<!-- enum-declarator = "enum" ( -->
<!--                       | ident? "{" enum-list? "}" -->
<!--                       | ident ( "{" enum-list? "}" )? -->
<!--                       ) -->

<!-- enum-list = ident ( "=" num )? ( "," ident ( "=" num )? )* ","? -->

<!-- type-suffix = ( "[" num "]" )* -->
<!--              | "(" funcdef-args? ")" -->
<!-- funcdef-args = param ( "," param )* -->
<!-- param = typespec declarator -->
<!-- declaration = typespec declarator -->
<!--               ( "=" initializer )? -->
<!--               ( "," declarator ( "=" initializer )? )* -->

<!-- initializer = assign | "{" unary ( "," unary )* "}" -->
<!-- compound-stmt = "{" ( declaration ";" | stmt )* "}" -->

<!-- stmt = expr ";" -->
<!--      | compound-stmt -->
<!--      | "if" "(" expr ")" stmt ( "else" stmt )? -->
<!--      | "while" "(" expr ")" stmt -->
<!--      | "for" "(" ( declaration | expr )? ";" expr? ";" expr? ")" stmt -->
<!--      | "return" expr ";" -->

<!-- expr = assign -->

<!-- assign = conditional ( -->
<!--          ( "=" | "+=" | "-=" | "*=" | "/=" ) assign -->
<!--          | ( "++" | "--" ) -->
<!--        )? -->

<!-- conditional = logor ( "?" expr ":" conditional )? -->

<!-- logor = logand ( "||"  logand )* -->
<!-- logand = bitor ( "&&"  bitor )* -->
<!-- bitor = bitxor ( "|" bitxor )* -->
<!-- bitxor = bitand ( "^" bitand *) -->
<!-- bitand = equality ( "&" equality )* -->

<!-- equality = relational ( "==" relational | "!=" relational )* -->

<!-- relational = add ( "<" add | "<=" add | ">" add | ">=" add )* -->

<!-- add = mul ( "+" mul | "-" mul )* -->
<!-- mul = cast ( "*" cast | "/" cast )* -->
<!-- cast = ( "(" typename ")" )? unary -->
<!-- unary = ( "+" | "-" | "*" | "&" | "~" )? cast -->
<!--       | ( "++" | "--" ) unary -->
<!--       | postfix -->

<!-- postfix = primary -->
<!--         | ( "[" expr "]" )* -->
<!--         | "." ident -->
<!--         | "->" ident -->
<!-- primary = "(" "{" compound-stmt "}" ")" -->
<!--         | num -->
<!--         | ident funcall-args? -->
<!--         | "(" expr ")" -->
<!--         | sizeof unary -->
<!--         | string-literal* -->
<!-- funcall = ident "(" funcall-args ")" -->
<!-- funcall-args = assign ( "," assign )* -->
<!-- ``` -->
