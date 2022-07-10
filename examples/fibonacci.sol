func fib(n: i64) -> i64 {
    return if n < 3 {
        1
    } else {
        fib(n-1) + fib(n-2)
    }
}

func main() -> i64 {
    printf("fibonacci(%d) is %d\n", 8, fib(8))
    return 0
}
