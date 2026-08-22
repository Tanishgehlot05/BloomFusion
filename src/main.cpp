#include <iostream>
#include "Database.h"

/*
 * main.cpp
 *
 * Menu-driven interface for the BloomFusion Database.
 *
 * Options:
 *   1. Insert a key-value pair
 *   2. Get a value by key (shows Bloom Filter + Fusion Tree steps)
 *   3. Delete a key
 *   4. Print all records
 *   5. Print database stats
 *   6. Exit
 */

void printMenu()
{
    std::cout << "\n======================================\n";
    std::cout << "    BloomFusion Database - Menu\n";
    std::cout << "======================================\n";
    std::cout << " 1. Insert (key, value)\n";
    std::cout << " 2. Get value by key\n";
    std::cout << " 3. Delete key\n";
    std::cout << " 4. Print all records (sorted)\n";
    std::cout << " 5. Show database stats\n";
    std::cout << " 6. Exit\n";
    std::cout << "--------------------------------------\n";
    std::cout << " Choice: ";
}

int main()
{
    std::cout << "\n  Welcome to the BloomFusion Database!\n";
    std::cout << "  This database uses:\n";
    std::cout << "    - Bloom Filter  (probabilistic pre-filter using FNV-1a hash)\n";
    std::cout << "    - Fusion Tree   (B-tree with wide fusion nodes, O(log_B n) search)\n\n";

    // Initial capacity estimate (just for Bloom Filter sizing, tree grows dynamically)
    int cap = 0;
    std::cout << "  Enter expected number of elements (for Bloom Filter sizing): ";
    std::cin >> cap;
    if (cap <= 0) cap = 1000;

    Database db(cap);

    int choice = 0;

    while (true)
    {
        printMenu();
        std::cin >> choice;

        if (choice == 1)
        {
            long long key = 0;
            long long val = 0;
            std::cout << "  Enter key   : ";
            std::cin >> key;
            std::cout << "  Enter value : ";
            std::cin >> val;

            db.insert(key, val);
            std::cout << "  Inserted (" << key << ", " << val << ") successfully.\n";
        }
        else if (choice == 2)
        {
            long long key = 0;
            std::cout << "  Enter key to look up: ";
            std::cin >> key;

            long long result = 0;
            bool found = db.get(key, result);

            if (!found)
                std::cout << "  Result: Key NOT in database.\n";
            else
                std::cout << "  Result: Found! Value = " << result << "\n";
        }
        else if (choice == 3)
        {
            long long key = 0;
            std::cout << "  Enter key to delete: ";
            std::cin >> key;
            db.remove(key);
        }
        else if (choice == 4)
        {
            db.printAll();
        }
        else if (choice == 5)
        {
            db.printStats();
        }
        else if (choice == 6)
        {
            std::cout << "\n  Goodbye!\n\n";
            break;
        }
        else
        {
            std::cout << "  Invalid choice. Please enter 1-6.\n";
        }
    }

    return 0;
}
