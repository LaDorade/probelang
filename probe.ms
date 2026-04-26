divide :: (a: number, b: number) number!string {
    if (b == 0) {
        reject "Could not 
        divide by 0"
    }

    return a / b;
}
export main :: () void {
    let res = divide(30, 2) catch err {
        print(err)
        local return 0
    }

    print("Result:", res)
}

