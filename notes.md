# Reflexions diverses

Petit snippet du langage :

```javascript
main :: (): void {
    print("hello world");

    let y = 0;
    let res = divide(32, y) catch err {
        print("ERROR: {}", err);
        local return 0;
    }
    print("32 / {} = {}", y, res);
}

divide :: (x: i32, y: i32) i32!string {
    if y == 0 {
        reject "Could not divide by 0";
    }
    return x / y;
}
```

## Testing

Faire un petit framework de test e2e et unitaire


## Funcall

Les appels de fonction `add(...)` sont des **expressions**.

## Blocks

Les blocks `{ ... }` ne peuvent pas être des **expressions**.

Cela pourrait donner sinon des choses comme :

```javascript
print({return "hello"}, "world");
```

## Les blocks comme expressions

MAIS en vrai ça pourrait être fun. Le soucis c'est les cas comme ceci :

```javascript
main :: () {
    return {
        let a = 32;
        if (a < 30) {
            local reject;
        }
        local return a + 32;
    }
}
```
Ici, quel est le type de retour de main ?
-> on ne reject pas directement
-> mais dans un même temps, on return un number et void

Solutions ? :

reject devient un modificateur de return, et se propage vers le haut:
`[local] reject ...` == `[local] [reject] return ...`

Autre problème que je viens de voir : `local` est censé être *local* au block, mais dans le cas d'un `if`, on fait quoi ?
c'est le if qui peut retourner qqch (j'aime bien) ? mais dans ce cas, commment je retourne pour le block du dessus mais pas la fonction ?

on pourrait par exemple :

```javascript
main :: () {
    return {
        let a = 32;
        return if (a < 30) {
            local reject;
        }
        local return a + 32; // <- rechable si la cond est fausse
    }
}
```

### Localité des blocks

on va parler des blocks "inconditionnel". Ces blocks peuvent retourner des
values et sont similaire à des expressions -- mais son moins permissif
dans leurs placements --.

Ex:

```javascript
main :: () { // block a.k.a. scope de la fonction
    let a = 42; // global à la fonction

    { // block interne, possédans son propre scope (mais toujours dans la fonction)
        a = 32; // accès à "a" !
        let b = 20; // local au block
        if (a < b) {
            return; // on quitte la fonction
        }
    }

    print(a); // 32

    // ...
}
```

C'est l'exemple typique des blocks en pogramation.
Maintenant prenons ça :

```javascript
main :: () { // block a.k.a. scope de la fonction
    let a = 42; // global à la fonction
    let b = { // block interne, avec son propre scope
        // ... des computations complexes
        let x = ...
        local return x; // ! on quitte le block avec une valeur !
    }

    print(b) // x
}
```

Le block qui permet d'assigner la valeur est un block inconditionnel,
on passera dedans quoi qu'il arrive. Ici, le block agit comme un block plus haut, mais permet de renvoyer une valeur via `local return` (modificateur 
`local` pour `return`)

Il agit ici comme une expression, cela va de soit qu'il doit donc systèmatiquement renvoyer une valeur;


Les blocks d'assignation peuvent donc renvoyer des valeurs à leur scope du dessus,
ainsi que return pour quitter la fonction courante.

Les blocks `if`, `while` et `switch` sont des blocks conditionnels et ne peuvent pas renvoyer des valeurs via `local`.

### Le cas des labels

```zig
fun main: () -> void = { // block a.k.a. scope de la fonction
    block: {
        ...
        local return block;
    }
    printf("bijour\n");
    let a = assign: {
        let x = 23;
        let y = 2;
        lab: while (true) {
            ...
            if (x == 12) {
                // return | will return from function main
                // local return | will return from 'lab' block
                // break assign x | why not
                // local return assign x | horrible
                // local return :assign x | clear ?
                // return :assign x | since we specify a block, no need for 'local'
                return :assign x; // feels nice
                return:assign x; // ?

                // maybe ?
                :assign return x;
            } else if (x == 1) {
                // break lab;
                // :lab return;
                return :lab;
            } else if (x == 0) { // want to return from function
                return;
            }
        }
        // local return y; | was the original idea
        // return local y; | follow the same pattern as above
        // return :local y; | better ? follow the above rule
        return :local y; // above pattern, but local is reserved
        return:local y;
        // why not make is a directive / builtin ?
        // return #local y;
        // but we want to specify the RETURN, not the expression so
        // #local return y; // seems better to fit the intent
        // :local return y; // local is reserved block label
    }
}
```
After testing, `'return' [ ':' { ident } ] [expression] ;` seems to be the best fit.
Where "local" is a keyword that be placed after ':' to return from the first inconditionnal block
(The same applies for reject)





