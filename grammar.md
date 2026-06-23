# Grammar

Setting up the grammar and ideas behind.

## Error handling

A value can be of sort "Success" or of sort "Error". When explicitly defining a value, its denoted
like this : 
```javascript
let x: success_type!error_type;
// less abstract :
let y: i32!string;
```
The value IS of type success AND error, this must be checked at runtime. Under the hood I plan this
to be a simple tagged union of the 2 types.
Thus, if a value is in this state, you must handle the error case (or else types wont match), so
```javascript
let y: i32!string = getNumber();
printf("{ y + 34 }\n"); // type error: type 'i32!string' not assignable to type number
```
won't work.

### Functions

Like in some other programming languages, we have 2 distincts path for what type of value we want
to return from the function. For the happy path, it's like always, `return`. On the other hand,
when things goes wrong (like every time in life) you use the keyword `reject`.

Example
```javascript
fun divide: (a: i32, b: i32) -> i32!string = {
    if b == 0
        reject "Cant divide by 0";
    return a / b;
}
let y: i32!string = divide(1, 2);
```

### Strategies to handle errors

#### Keywords
```javascript
let y: i32!string = getNumber() catch {
    return:local 0; // if we it the error, we fall on this block
};
printf("{ y + 34 }\n"); // all good
```

```javascript
let y: i32!string = try getNumber(); // this will bubble the error to the upper function scope
printf("{ y + 34 }\n"); // all good
```

---- RANDOM STUFF

const st: struct = { } // mandatory to indicate type "struct"
-> const is useless
const en: enum = { } // mandatory to indicate type "enum"
-> const is useless
const fn: (i32) -> void = { } // mandatory to indicate type "(...) -> ..."
-> const is useless
// const fn: fun (i32) -> void = { }
const va: i32 = 34; // not mandatory, but declares mutability or not
const va = 34;
-> const is necessary
let va: i32 = 34; // not mandatory, but declares mutability or not
let va = 34;

trying to conciliate mutability, modifier, data type, et execution

/// ideas for variables
va: var(i32) = 32; // declares variable as mutable
va: i32 = 32; // declares variable as const
// inference:
va = x(); // infer va as const(ret_type of x)
va: var = infere va as mutable(ret_type of x)


plus ça va plus je me rapproche de:
st : struct : { }     // ici le type serait "struct" uniquement -> pas bon 
st :: struct { }      // ici le type est inféré car il suit ce qui suit
en : enum : { }       // ici le type serait "enum" uniquement -> pas bon
en :: enum { }        // ici le type est inféré car défini après
fn : () -> void : { } // le type est le prototype
va :: 32              // constant inféré
va : i32 : 32         // constant
va := 32              // mutable inféré
va : i32 = 32         // mutable

=> on confond 2 choses, l'assignation de type et l'assignation de valeur
ça peut théoriquement être la même chose mais ça peut créer de la confusion

- il faut pouvoir créer des valeurs -> évident
    - mutable et non mutable
    - est résolvable au runtime ou au comptime
- il faut pouvoir créer des types -> pour spécialiser la donnée
    - n'est que constant
    - n'est que comptime ?
- il faut pouvior créer des fonctions -> pour executer le code
    - n'est que constant et comptime

variables:
v :: 32
v : i32 : 32
v := 24
v : i32 = 24
types:
t @= struct {}
function



struct/enum/union are fondamentally different thant functions
struct (& others) produces TYPES that are not know before writing
functions, produces something(?) but this is already typed (denoted by the type annotation)

fun fn: (i32) -> void = {...}
=> fn: (i32) -> void // fn got the TYPE "(i32) -> void"

struct st = {a:i32,b:f46,...}
=> st: {a:i32,b:f46,...} // st got the TYPE "{a:i32,b:f46,...}"
-> struct melts into a underlying data layout

enum en = {A,B,C,...}
=> en: {A,B,C,...} // en got the type "{A,B,C,...}"

let v = 32;
v: i32 // 

struct Testing = {
    fn: fun: (u32, i32) -> i32,
    fn: (u32, i32) -> i32,
    st: struct = { ... },
    en: enum = { }
}

struct Token = {
    row: u32,
    col: u32
}

enum Lexeme(Token) = { // chaque variant à les champs de Token
    Ident = { // les variant peuvent avoir des champs perso
        val: string;
    },
    Number: struct = { // mieux ?, traduit mieux ce qu'on peut mettre ?
        val: f64;
    },
    Local,
    ...
}
// on peut pas permettre n'importe quel type de donnée comme valeur "racine" d'un champ d'enum.
// comprendre: pas de "struct = " mais simplement '='
// on peut voir le type des champs comme une "enum's struct"
// pourra plus tard si besoin/envie/temps avoir un tuple ou enum

struct Funcdef = {
    name: Lexeme::Ident,
    ret_type: ...,
    body: ...,
}

enum Stmt = {
    funcdef: Funcdef
    expr_stmt: Expr
}

Token :: struct {
    ...
}
Lexeme :: enum(Token) {
    Ident: struct {
        ...
    },
    Local: proc() // comment call la fonction ??
}

Lexeme::Local() ?

let t = Lexeme::Ident {
    row: 0,
    col: 23,
    val: "Bijour",
};

---- END RANDOM STUFF
