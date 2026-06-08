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


#### Pattern matching

Not in my mind yet
