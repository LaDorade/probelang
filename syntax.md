# Syntax

À la base je voulais utiliser `main :: ()` comme syntax pour déclarer les fonctions (inspiré par Odin ou Jai)
je trouve la syntax élégante et clair.
Cependant, cela dénote avec l'utilisation de keyword pour déclarer les strctures, types ou les variables :
``` javascript
const x = ...
let x = ...
struct x = {
    ...
}
```
Donc, je suis repartie sur quelque chose de plus classique mais surtout cohérent :
```javascript
fun x(...) {...}
```.
La question est maintenant de savoir comment rendre le tout encore plus cohérent et clair car :

On peut expand chaque déclaration vu plus haut comme ceci :
``` javascript
const x: i32 = 32;
let x: string = "snoup";
struct x: Type = {
    ...
}
```

Options: 

1:
```python
# Declaration / Definition
fun x: (a: i32, b: string): string = { ... }
# En tant que type :
struct ma_struct = {
    function: (i32, string): string
}
```
Avantages:
- clean
- simple,
- cohérent

Inconvénients:
- inhabituel
- les ':' un peu partout peut rendre le tout confus

2:
```zig
# Declaration / Definition
fun x: (a: i32, b: string) -> string = { ... }
# En tant que type :
struct ma_struct = {
    function: (i32, string) -> string
}
fun y: (
    fn: (i32) -> string!error
) -> string = { ... }
```
Avantages:
- clean
- simple,
- cohérent
- on voit bien la "production" de la fonction

Inconvénients:
- inhabituel
- pas de mot clé quand on l'utilise en tant que type => peut être relou à lire
    - mais permet d'avoir quelque chose de cohérent par rapport aux autre types

On peut imaginer le type des fonctions comme étant `'(' type { ',' type } ')' '->' type`
là où une variable est de type `type`;
une struct de type `Type`;
donc notre type vaut :
```javascript
type = 'Type'
     | type-expr
     | func-type
;
type-expr = ident [ '!' ident ] ;
func-type = '(' type { ',' type } ')' '->' type ;
func-decl = 'fun' ident ':' '(' ident ':' type { ',' ident ':' type } ')' '->' type '=' '{' { statements } '}';
```

#### Reflexion
> Il est interessant de voir que la fonction dans la description ci-dessus peut prendre 'type' en param et production, donc,
> que l'on peut avoir des choses qui n'ont de sens qu'à la compilation (comme 'Type')
> On pourrait soit prendre en considération ce comportement (comme en zig), ou soit complétement ignoré ça et mettre 'type-expr'
> à la place


## Structures

Comment gérer les structures/enum/union anonymes ?

A:
```odin
struct ma_struct = {
    x: i32;
    y: f64;
    s: struct = { // ne pas mettre de nom pour déclarer la struct ?
                  // implique que l'on attend aussi une struct aux endroit d'un type-expr
        a: i32;
        b: i32;
    };
    u: union = {
        a: i32;
        b: u8;
    }
    s2: struct: Type = { // :(
                         // zig has something similar and doesn't allow ': Type'
                         // so why not !
        a: i32;
    };
    u2: union: Type = {
        a: i32;
        b: u8;
    }
    // this require to modify the parsing rule of struct
    // so
    // 'struct' ident [ ':' ident ] '=' '{' ... '}'
    // become
    // 'struct' [ ident ] [ ':' ident ] '=' '{' ... '}'
    // not a big deal but my heart is broken that its not 'that' clean of a syntax
}
```
```zig
const Sus: type = struct { // allow ': type' here
    a: i32,
    b: i32,
    u: union(u8): type { // not here
        a: i32,
        b: i32,
    }
};
// This has the advantage of only requiring one syntax for parsing both top level
// and nested declaration
```

B:
```c
struct ma_struct = {
    x: i32;
    y: f64;
    struct s = { // act as anon Type struct, but is also a field
                 // not really clear
        a: i32;
    };
    union u = {
        a: i32;
        b: u8;
    }
    struct s2: Type = { // equivalent
        a: i32;
    };
    union u2: Type = {
        a: i32;
        b: u8;
    }
}

