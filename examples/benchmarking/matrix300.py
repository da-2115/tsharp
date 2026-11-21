
def main():
    n = 300

    A = [1.0] * (n * n)
    B = [2.0] * (n * n)
    C = [0.0] * (n * n)

    # Matrix multiplication: C = A * B
    for i in range(n):
        for j in range(n):
            total = 0.0

            for k in range(n):
                total += A[i * n + k] * B[k * n + j]

            C[i * n + j] = total

    # Verify result
    print(C[0])
    print(C[n * n - 1])


if __name__ == "__main__":
    main()
