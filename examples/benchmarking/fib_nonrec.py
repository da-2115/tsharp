
def main():
    result = 0

    for j in range(1000000):
        a = 0
        b = 1

        for i in range(35):
            next_value = a + b
            a = b
            b = next_value

        result = a

    print(result)

if __name__ == "__main__":
    main()
