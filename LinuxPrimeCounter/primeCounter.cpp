#include <iostream>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <vector>
#include <cstring>

using namespace std;

// Function to check if a number is prime
bool isPrime(int num) {
    if (num < 2) return false;
    if (num == 2) return true;
    if (num % 2 == 0) return false;
    for (int i = 3; i * i <= num; i += 2) {
        if (num % i == 0) return false;
    }
    return true;
}

// Child process function
void childProcess(int processId, int startRange, int endRange, int writeFd) {
    // We only need to write
    
    vector<int> primes;
    
    // Search for primes in the assigned range
    for (int i = startRange; i < endRange; i++) {
        if (isPrime(i)) {
            primes.push_back(i);
        }
    }
    
    // Send number of primes found
    int count = primes.size();
    write(writeFd, &count, sizeof(count));
    
    // Send all primes
    for (int prime : primes) {
        write(writeFd, &prime, sizeof(prime));
    }
    
    close(writeFd);
    exit(0);
}

int main() {
    cout << "=== Prime Number Finder (Linux/Mac) ===" << endl;
    cout << "Searching for primes from 1 to 10,000 using 10 processes" << endl << endl;
    
    const int NUM_PROCESSES = 10;
    const int RANGE_SIZE = 1000;
    const int MAX_NUMBER = 10000;
    
    vector<pid_t> childPids;
    vector<int> readFds;
    vector<int> writeFds;
    
    // Create pipes and child processes
    for (int i = 0; i < NUM_PROCESSES; i++) {
        int pipeFds[2];
        
        // Create pipe
        if (pipe(pipeFds) == -1) {
            perror("pipe");
            exit(1);
        }
        
        int startRange = i * RANGE_SIZE + 1;
        int endRange = (i + 1) * RANGE_SIZE + 1;
        
        pid_t pid = fork();
        
        if (pid == -1) {
            perror("fork");
            exit(1);
        }
        
        if (pid == 0) {
            // Child process
            close(pipeFds[0]);  // Close read end
            childProcess(i, startRange, endRange, pipeFds[1]);
        } else {
            // Parent process
            close(pipeFds[1]);  // Close write end
            childPids.push_back(pid);
            readFds.push_back(pipeFds[0]);
        }
    }
    
    // Collect results from all child processes
    vector<int> allPrimes;
    
    for (int i = 0; i < NUM_PROCESSES; i++) {
        int count;
        
        // Read number of primes from this child
        if (read(readFds[i], &count, sizeof(count)) == -1) {
            perror("read");
            continue;
        }
        
        cout << "Process " << i << " (" << (i * RANGE_SIZE + 1) << "-" 
             << ((i + 1) * RANGE_SIZE) << "): Found " << count << " primes" << endl;
        
        // Read all primes from this child
        for (int j = 0; j < count; j++) {
            int prime;
            if (read(readFds[i], &prime, sizeof(prime)) == -1) {
                perror("read");
                continue;
            }
            allPrimes.push_back(prime);
        }
        
        close(readFds[i]);
    }
    
    // Wait for all child processes to finish
    for (int i = 0; i < NUM_PROCESSES; i++) {
        waitpid(childPids[i], nullptr, 0);
    }
    
    // Display all primes found
    cout << endl << "=== All Prime Numbers Found ===" << endl;
    for (size_t i = 0; i < allPrimes.size(); i++) {
        cout << allPrimes[i];
        if (i < allPrimes.size() - 1) {
            cout << ", ";
            if ((i + 1) % 10 == 0) cout << endl;
        }
    }
    cout << endl << endl << "Total primes found: " << allPrimes.size() << endl;
    
    return 0;
}
