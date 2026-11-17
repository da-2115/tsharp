def fib(n):
    if n <= 1:
        return n

    return fib(n - 1) + fib(n - 2)

def main():
    result = fib(35)
    print(result)

if __name__ == "__main__":
    main()
