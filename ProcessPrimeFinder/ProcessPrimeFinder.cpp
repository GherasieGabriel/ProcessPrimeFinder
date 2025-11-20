#undef UNICODE
#undef _UNICODE
#include <windows.h>
#include <iostream>
#include <vector>

using namespace std;

bool isPrime(int num) {
    if (num < 2) return false;
    if (num == 2) return true;
    if (num % 2 == 0) return false;
    for (int i = 3; i * i <= num; i += 2) {
        if (num % i == 0) return false;
    }
    return true;
}

int main(int argc, char* argv[]) {
    // Check if child process
    if (argc == 4) {
        int start = atoi(argv[1]);
        int end = atoi(argv[2]);
        HANDLE hWrite = (HANDLE)(uintptr_t)strtoull(argv[3], NULL, 16);

        vector<int> primes;
        for (int j = start; j < end; j++) {
            if (isPrime(j)) primes.push_back(j);
        }

        DWORD bytesWritten;
        int count = primes.size();
        WriteFile(hWrite, &count, sizeof(count), &bytesWritten, NULL);
        for (int p : primes) {
            WriteFile(hWrite, &p, sizeof(p), &bytesWritten, NULL);
        }
        CloseHandle(hWrite);
        return 0;
    }

    // Parent process
    const int NUM_PROCESSES = 10;
    const int RANGE_SIZE = 1000;

    vector<HANDLE> hProcesses;
    vector<HANDLE> hReadPipes;

    SECURITY_ATTRIBUTES sa;
    sa.nLength = sizeof(SECURITY_ATTRIBUTES);
    sa.bInheritHandle = TRUE;
    sa.lpSecurityDescriptor = NULL;

    // Get this executable's name
    char exePath[MAX_PATH];
    GetModuleFileNameA(NULL, exePath, MAX_PATH);

    // Create processes
    for (int i = 0; i < NUM_PROCESSES; i++) {
        HANDLE hRead, hWrite;
        CreatePipe(&hRead, &hWrite, &sa, 0);

        HANDLE hDup;
        DuplicateHandle(GetCurrentProcess(), hRead,
            GetCurrentProcess(), &hDup, 0, FALSE,
            DUPLICATE_SAME_ACCESS);
        CloseHandle(hRead);
        hRead = hDup;

        int start = i * RANGE_SIZE + 1;
        int end = (i + 1) * RANGE_SIZE + 1;

        STARTUPINFOA si;
        PROCESS_INFORMATION pi;
        ZeroMemory(&si, sizeof(si));
        ZeroMemory(&pi, sizeof(pi));
        si.cb = sizeof(si);
        si.dwFlags = STARTF_USESTDHANDLES;
        si.hStdOutput = hWrite;
        si.hStdError = hWrite;
        si.hStdInput = GetStdHandle(STD_INPUT_HANDLE);

        char cmd[512];
        sprintf_s(cmd, sizeof(cmd), "%s %d %d %llx", exePath, start, end, (unsigned long long)hWrite);

        BOOL success = CreateProcessA(NULL, cmd, NULL, NULL, TRUE, 0, NULL, NULL, &si, &pi);
        if (!success) {
            cerr << "CreateProcess failed for process " << i << endl;
            continue;
        }

        CloseHandle(pi.hThread);
        CloseHandle(hWrite);

        hProcesses.push_back(pi.hProcess);
        hReadPipes.push_back(hRead);
    }

    // Collect results
    cout << "Primes found:\n";
    int total = 0;

    for (int i = 0; i < NUM_PROCESSES; i++) {
        DWORD bytesRead;
        int count;
        if (!ReadFile(hReadPipes[i], &count, sizeof(count), &bytesRead, NULL)) {
            cerr << "Failed to read from process " << i << endl;
            continue;
        }

        cout << "Process " << i << ": " << count << " primes\n";

        for (int j = 0; j < count; j++) {
            int prime;
            ReadFile(hReadPipes[i], &prime, sizeof(prime), &bytesRead, NULL);
            cout << prime << " ";
        }
        cout << "\n";

        total += count;
        CloseHandle(hReadPipes[i]);
    }

    WaitForMultipleObjects(NUM_PROCESSES, hProcesses.data(), TRUE, INFINITE);

    for (int i = 0; i < NUM_PROCESSES; i++) {
        CloseHandle(hProcesses[i]);
    }

    cout << "\nTotal: " << total << " primes\n";
    return 0;
}