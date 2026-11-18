// ============================================================
// Bank Account Management System
// Subject: OOP using C++ (CIC-211)
// Team   : [Student 1 Name] & [Student 2 Name]
// Concepts: Classes/Objects, Encapsulation, Constructors,
//           Destructor, Inheritance, Abstract class, Virtual
//           functions, Function overriding, Function overloading,
//           Custom exception handling, File handling, Vector
// ============================================================
#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <string>
#include <limits>
#include <iomanip>
#include <stdexcept>
using namespace std;

// ---------- Custom exception for banking rule violations ----------
class BankException : public runtime_error {
public:
    BankException(const string &msg) : runtime_error(msg) {}
};

// ---------- Helper functions for safe input ----------
int readInt(const string &msg, int lo, int hi) {
    int x;
    while (true) {
        cout << msg;
        if (cin >> x && x >= lo && x <= hi) {
            cin.ignore(numeric_limits<streamsize>::max(), '\n');
            return x;
        }
        cin.clear();
        cin.ignore(numeric_limits<streamsize>::max(), '\n');
        cout << "Invalid input! Enter a number between " << lo << " and " << hi << ".\n";
    }
}

double readDouble(const string &msg, double lo, double hi) {
    double x;
    while (true) {
        cout << msg;
        if (cin >> x && x >= lo && x <= hi) {
            cin.ignore(numeric_limits<streamsize>::max(), '\n');
            return x;
        }
        cin.clear();
        cin.ignore(numeric_limits<streamsize>::max(), '\n');
        cout << "Invalid amount! Enter a valid number (not negative).\n";
    }
}

string readLine(const string &msg) {
    string s;
    while (true) {
        cout << msg;
        getline(cin, s);
        if (!s.empty() && s.find('|') == string::npos && s.find(';') == string::npos)
            return s;
        cout << "Invalid input! Cannot be empty or contain '|' or ';'.\n";
    }
}

// ============================================================
// Base class: Account (abstract, because of pure virtual functions)
// ============================================================
class Account {
private:                                   // Encapsulation
    int accNo;
    string name;
    double balance;
    vector<string> history;                // simple transaction history

protected:
    // Only the account classes can change the balance directly
    void record(const string &kind, double amt) {
        ostringstream os;
        os << fixed << setprecision(2) << kind << " Rs." << amt
           << " (Balance Rs." << balance << ")";
        history.push_back(os.str());
    }
    void credit(double amt, const string &kind) { balance += amt; record(kind, amt); }
    void debit(double amt, const string &kind)  { balance -= amt; record(kind, amt); }

public:
    // Constructor
    Account(int no, const string &n, double openingBalance)
        : accNo(no), name(n), balance(openingBalance) {
        record("Account opened with", openingBalance);
    }
    virtual ~Account() {}                  // virtual destructor

    // Getters
    int getAccNo() const { return accNo; }
    string getName() const { return name; }
    double getBalance() const { return balance; }

    // Function overloading: two versions of deposit()
    void deposit(double amt) { deposit(amt, "cash"); }
    void deposit(double amt, const string &note) {
        if (amt <= 0) throw BankException("Deposit amount must be greater than zero.");
        credit(amt, "Deposit (" + note + ")");
    }

    void withdraw(double amt) {
        if (amt <= 0) throw BankException("Withdrawal amount must be greater than zero.");
        if (balance - amt < minBalance()) {
            ostringstream os;
            os << fixed << setprecision(2)
               << "Insufficient balance! Minimum balance Rs." << minBalance()
               << " must remain. Maximum you can withdraw: Rs."
               << (balance - minBalance() > 0 ? balance - minBalance() : 0);
            throw BankException(os.str());
        }
        debit(amt, "Withdrawal");
    }

    // Pure virtual functions: every account type must override these
    virtual double minBalance() const = 0;
    virtual string type() const = 0;
    virtual void monthEndUpdate() = 0;

    void display() const {
        cout << fixed << setprecision(2)
             << "Account No   : " << accNo << "\n"
             << "Holder Name  : " << name << "\n"
             << "Account Type : " << type() << "\n"
             << "Balance      : Rs." << balance << "\n";
    }

    void showHistory() const {
        cout << "--- Transaction history of account " << accNo << " ---\n";
        for (size_t i = 0; i < history.size(); i++)
            cout << i + 1 << ". " << history[i] << "\n";
    }

    // File support
    string toFileString() const {
        ostringstream os;
        os << fixed << setprecision(2)
           << type() << '|' << accNo << '|' << name << '|' << balance << '|';
        for (size_t i = 0; i < history.size(); i++) {
            if (i) os << ';';
            os << history[i];
        }
        return os.str();
    }
    void restoreHistory(const string &h) {
        history.clear();
        stringstream ss(h);
        string item;
        while (getline(ss, item, ';'))
            if (!item.empty()) history.push_back(item);
    }
};

// ============================================================
// Derived class 1: SavingsAccount
// ============================================================
class SavingsAccount : public Account {
public:
    SavingsAccount(int no, const string &n, double bal) : Account(no, n, bal) {}
    double minBalance() const override { return 500.0; }
    string type() const override { return "Savings"; }
    void monthEndUpdate() override {              // 0.5% monthly interest
        double interest = getBalance() * 0.005;
        if (interest > 0) credit(interest, "Interest credited");
    }
};

// ============================================================
// Derived class 2: CurrentAccount
// ============================================================
class CurrentAccount : public Account {
public:
    CurrentAccount(int no, const string &n, double bal) : Account(no, n, bal) {}
    double minBalance() const override { return 0.0; }
    string type() const override { return "Current"; }
    void monthEndUpdate() override {              // Rs.50 maintenance fee
        if (getBalance() >= 50.0) debit(50.0, "Maintenance fee");
    }
};

// ============================================================
// Bank class: manages all accounts
// ============================================================
class Bank {
private:
    vector<Account *> accounts;                   // base-class pointers (polymorphism)
    const string fileName = "accounts.txt";

    Account *findAccount(int no) const {
        for (size_t i = 0; i < accounts.size(); i++)
            if (accounts[i]->getAccNo() == no) return accounts[i];
        return nullptr;
    }

    int nextAccNo() const {
        int mx = 1000;
        for (size_t i = 0; i < accounts.size(); i++)
            if (accounts[i]->getAccNo() > mx) mx = accounts[i]->getAccNo();
        return mx + 1;
    }

public:
    Bank() { loadFromFile(); }
    ~Bank() {                                     // Destructor: free memory
        for (size_t i = 0; i < accounts.size(); i++) delete accounts[i];
    }
    Bank(const Bank &) = delete;                  // no copying of Bank
    Bank &operator=(const Bank &) = delete;

    void loadFromFile() {
        ifstream fin(fileName);
        if (!fin) return;                         // first run: no file yet
        string line;
        while (getline(fin, line)) {
            if (line.empty()) continue;
            try {
                stringstream ss(line);
                string t, no, n, bal, hist;
                getline(ss, t, '|'); getline(ss, no, '|'); getline(ss, n, '|');
                getline(ss, bal, '|'); getline(ss, hist);
                Account *a;
                if (t == "Savings") a = new SavingsAccount(stoi(no), n, stod(bal));
                else if (t == "Current") a = new CurrentAccount(stoi(no), n, stod(bal));
                else throw runtime_error("unknown type");
                a->restoreHistory(hist);
                accounts.push_back(a);
            } catch (const exception &) {
                cout << "Warning: skipped a corrupted record in file.\n";
            }
        }
    }

    void saveToFile() const {
        ofstream fout(fileName);
        if (!fout) { cout << "Error: could not save to file!\n"; return; }
        for (size_t i = 0; i < accounts.size(); i++)
            fout << accounts[i]->toFileString() << "\n";
    }

    void createAccount() {
        cout << "1. Savings Account (minimum balance Rs.500)\n"
             << "2. Current Account (no minimum balance)\n";
        int t = readInt("Choose account type: ", 1, 2);
        string name = readLine("Enter customer name: ");
        double bal = readDouble("Enter opening balance: ", 0, 10000000);
        int no = nextAccNo();

        Account *a;
        if (t == 1) a = new SavingsAccount(no, name, bal);
        else        a = new CurrentAccount(no, name, bal);

        if (bal < a->minBalance()) {
            cout << "Error: opening balance must be at least Rs."
                 << a->minBalance() << " for this account type.\n";
            delete a;
            return;
        }
        accounts.push_back(a);
        saveToFile();
        cout << "Account created successfully! Your account number is " << no << "\n";
    }

    void deposit() {
        Account *a = findAccount(readInt("Enter account number: ", 1, 999999));
        if (!a) { cout << "Account not found.\n"; return; }
        double amt = readDouble("Enter amount to deposit: ", 0, 10000000);
        try {
            a->deposit(amt);
            saveToFile();
            cout << fixed << setprecision(2)
                 << "Deposit successful. New balance: Rs." << a->getBalance() << "\n";
        } catch (const BankException &e) {
            cout << "Error: " << e.what() << "\n";
        }
    }

    void withdraw() {
        Account *a = findAccount(readInt("Enter account number: ", 1, 999999));
        if (!a) { cout << "Account not found.\n"; return; }
        double amt = readDouble("Enter amount to withdraw: ", 0, 10000000);
        try {
            a->withdraw(amt);
            saveToFile();
            cout << fixed << setprecision(2)
                 << "Withdrawal successful. New balance: Rs." << a->getBalance() << "\n";
        } catch (const BankException &e) {
            cout << "Error: " << e.what() << "\n";
        }
    }

    void showAccount() const {
        Account *a = findAccount(readInt("Enter account number: ", 1, 999999));
        if (!a) { cout << "Account not found.\n"; return; }
        a->display();
    }

    void showHistory() const {
        Account *a = findAccount(readInt("Enter account number: ", 1, 999999));
        if (!a) { cout << "Account not found.\n"; return; }
        a->showHistory();
    }

    void showAll() const {
        if (accounts.empty()) { cout << "No accounts found.\n"; return; }
        cout << left << setw(10) << "Acc No" << setw(20) << "Name"
             << setw(10) << "Type" << "Balance\n";
        cout << string(52, '-') << "\n";
        for (size_t i = 0; i < accounts.size(); i++)
            cout << left << setw(10) << accounts[i]->getAccNo()
                 << setw(20) << accounts[i]->getName()
                 << setw(10) << accounts[i]->type()
                 << fixed << setprecision(2) << accounts[i]->getBalance() << "\n";
    }

    // Runtime polymorphism: same call, different behaviour per account type
    void applyMonthEnd() {
        if (accounts.empty()) { cout << "No accounts found.\n"; return; }
        for (size_t i = 0; i < accounts.size(); i++)
            accounts[i]->monthEndUpdate();
        saveToFile();
        cout << "Month-end done: interest added to Savings, fee charged to Current accounts.\n";
    }
};

// ---------- main (kept short) ----------
int main() {
    Bank bank;
    int choice;
    do {
        cout << "\n===== BANK ACCOUNT MANAGEMENT SYSTEM =====\n"
             << "1. Create Account\n2. Deposit\n3. Withdraw\n"
             << "4. Search / Display Account\n5. Transaction History\n"
             << "6. Display All Accounts\n7. Month-End Update (Interest/Fee)\n8. Exit\n";
        choice = readInt("Enter your choice: ", 1, 8);
        switch (choice) {
            case 1: bank.createAccount(); break;
            case 2: bank.deposit(); break;
            case 3: bank.withdraw(); break;
            case 4: bank.showAccount(); break;
            case 5: bank.showHistory(); break;
            case 6: bank.showAll(); break;
            case 7: bank.applyMonthEnd(); break;
            case 8: cout << "Thank you! All data saved. Bye.\n"; break;
        }
    } while (choice != 8);
    return 0;
}
