
from concurrent.futures import ProcessPoolExecutor


def multiply_rows(n, start_row, end_row):
    total = 0.0

    for i in range(start_row, end_row):
        for j in range(n):
            value = 0.0

            for k in range(n):
                value += 1.0 * 2.0

            total += value

    return total


def main():
    n = 500

    with ProcessPoolExecutor(max_workers=4) as executor:
        a = executor.submit(multiply_rows, n, 0, 125)
        b = executor.submit(multiply_rows, n, 125, 250)
        c = executor.submit(multiply_rows, n, 250, 375)
        d = executor.submit(multiply_rows, n, 375, 500)

        total = (
            a.result()
            + b.result()
            + c.result()
            + d.result()
        )

    print(total)


if __name__ == "__main__":
    main()
