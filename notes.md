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

## Funcall

Les appels de fonction `add(...)` sont des **expressions**.

## Blocks

Les blocks `{ ... }` ne peuvent pas être des **expressions**.

Cela pourrait donner sinon des choses comme :

```javascript
print({return "hello"}, "world");
```
