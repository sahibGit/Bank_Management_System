#include <iostream>
//these headers are used to connect the code with database to perform sql queries
#include <cppconn/driver.h>
#include <cppconn/connection.h>
#include <cppconn/prepared_statement.h>
#include <cppconn/resultset.h>

#include <queue>
#include <vector>
#include <unordered_map>

using namespace std;
using namespace sql;

struct LoanApplication {
    int accountNum;
    double amount;
    int priority;
    bool approved = false;

    bool operator<(const LoanApplication& other) const {
        return priority < other.priority;
    }
};

class BankManagement {
private:
    Driver* driver;
    Connection* con;
    priority_queue<LoanApplication> loanQueue;
    unordered_map<int, vector<pair<int, double>>> transactionGraph;
    unordered_map<int, double> activeLoans; // accountNum -> remaining loan amount

public:
    BankManagement() {
        driver = get_driver_instance();
        con = driver->connect("tcp://127.0.0.1:3306", "root", "Sahibsql@1");
        con->setSchema("bank_db");
    }

    ~BankManagement() {
        delete con;
    }

    void AddAccount(const string& name, double balance) {
        PreparedStatement* pstmt = con->prepareStatement("INSERT INTO accounts(name, balance) VALUES (?, ?)");
        pstmt->setString(1, name);
        pstmt->setDouble(2, balance);
        pstmt->executeUpdate();
        delete pstmt;

        Statement* stmt = con->createStatement();
        ResultSet* res = stmt->executeQuery("SELECT LAST_INSERT_ID() AS account_num");
        if (res->next()) {
            int accNum = res->getInt("account_num");
            cout << "\t\tAccount Created Successfully! Assigned Account Number: " << accNum << endl;
        }
        delete res;
        delete stmt;
    }

    void showAllAccounts() {
        Statement* stmt = con->createStatement();
        ResultSet* res = stmt->executeQuery("SELECT * FROM accounts");

        cout << "\t\tAll Account Holders\n";
        while (res->next()) {
            int acc = res->getInt("account_num");
            cout << "Account Number: " << acc
                << " | Name: " << res->getString("name")
                << " | Balance: " << res->getDouble("balance");
            if (activeLoans.count(acc)) {
                cout << " | Loan Status: Active | Due: " << activeLoans[acc];
            }
            else {
                cout << " | Loan Status: None";
            }
            cout << endl;
        }

        delete res;
        delete stmt;
    }

    void searchAccount(int accountNum) {
        PreparedStatement* pstmt = con->prepareStatement("SELECT * FROM accounts WHERE account_num=?");
        pstmt->setInt(1, accountNum);
        ResultSet* res = pstmt->executeQuery();
        if (res->next()) {
            cout << "Account Found: " << res->getString("name")
                << ", Balance: " << res->getDouble("balance");
            if (activeLoans.count(accountNum)) {
                cout << ", Loan Status: Active, Due: " << activeLoans[accountNum];
            }
            else {
                cout << ", Loan Status: None";
            }
            cout << endl;
        }
        else {
            cout << "Account Not Found.\n";
        }
        delete res;
        delete pstmt;
    }

    void deposit(int accountNum, double amount) {
        PreparedStatement* pstmt = con->prepareStatement("UPDATE accounts SET balance = balance + ? WHERE account_num=?");
        pstmt->setDouble(1, amount);
        pstmt->setInt(2, accountNum);
        int rows = pstmt->executeUpdate();
        if (rows > 0)
            cout << amount << " Deposited Successfully.\n";
        else
            cout << "Account Not Found.\n";
        delete pstmt;
    }

    void withdraw(int accountNum, double amount) {
        PreparedStatement* check = con->prepareStatement("SELECT balance FROM accounts WHERE account_num=?");
        check->setInt(1, accountNum);
        ResultSet* res = check->executeQuery();
        if (res->next()) {
            double currentBalance = res->getDouble("balance");
            if (currentBalance >= amount) {
                PreparedStatement* withdraw = con->prepareStatement("UPDATE accounts SET balance = balance - ? WHERE account_num=?");
                withdraw->setDouble(1, amount);
                withdraw->setInt(2, accountNum);
                withdraw->execute();
                cout << "Withdrawn " << amount << " Successfully.\n";
                delete withdraw;
            }
            else {
                cout << "Insufficient Balance.\n";
            }
        }
        else {
            cout << "Account Not Found.\n";
        }
        delete res;
        delete check;
    }

    void deleteAccount(int accountNum) {
        PreparedStatement* pstmt = con->prepareStatement("DELETE FROM accounts WHERE account_num=?");
        pstmt->setInt(1, accountNum);
        int rows = pstmt->executeUpdate();
        if (rows > 0)
            cout << "Account " << accountNum << " deleted successfully.\n";
        else
            cout << "Account Not Found.\n";
        delete pstmt;
        activeLoans.erase(accountNum);
    }

    void transfer(int fromAcc, int toAcc, double amount) {
        PreparedStatement* check = con->prepareStatement("SELECT balance FROM accounts WHERE account_num=?");
        check->setInt(1, fromAcc);
        ResultSet* res = check->executeQuery();
        if (res->next()) {
            double bal = res->getDouble("balance");
            if (bal >= amount) {
                PreparedStatement* withdraw = con->prepareStatement("UPDATE accounts SET balance = balance - ? WHERE account_num=?");
                withdraw->setDouble(1, amount);
                withdraw->setInt(2, fromAcc);
                withdraw->execute();
                delete withdraw;

                PreparedStatement* deposit = con->prepareStatement("UPDATE accounts SET balance = balance + ? WHERE account_num=?");
                deposit->setDouble(1, amount);
                deposit->setInt(2, toAcc);
                deposit->execute();
                delete deposit;

                transactionGraph[fromAcc].push_back({ toAcc, amount });

                cout << "Transferred " << amount << " from Account " << fromAcc << " to Account " << toAcc << ".\n";
            }
            else {
                cout << "Insufficient balance in source account.\n";
            }
        }
        else {
            cout << "Source Account not found.\n";
        }
        delete res;
        delete check;
    }

    void applyLoan(int accountNum, double amount, int priority) {
        loanQueue.push({ accountNum, amount, priority });
        cout << "Loan Application Added.\n";
    }

    void processLoan() {
        if (!loanQueue.empty()) {
            LoanApplication top = loanQueue.top();
            loanQueue.pop();
            activeLoans[top.accountNum] = top.amount;
            cout << "Loan Approved: Account " << top.accountNum << ", Amount: " << top.amount << ", Priority: " << top.priority << endl;
        }
        else {
            cout << "No Loan Applications Pending.\n";
        }
    }

    void payLoan(int accountNum, double payment) {
        if (activeLoans.count(accountNum)) {
            double prev = activeLoans[accountNum];
            activeLoans[accountNum] -= payment;
            if (activeLoans[accountNum] <= 0) {
                cout << "Loan Payment: Rs. " << payment << " paid. Loan fully paid!\n";
                activeLoans.erase(accountNum);
            }
            else {
                cout << "Loan Payment: Rs. " << payment << " paid. Remaining Loan Due: " << activeLoans[accountNum] << endl;
            }
        }
        else {
            cout << "No active loan for this account.\n";
        }
    }

    void showTransactionHistory() {
        cout << "\nTransaction Graph:\n";
        for (const auto& pair : transactionGraph) {
            int from = pair.first;
            for (const auto& toPair : pair.second) {
                cout << "From account " << from << " to account " << toPair.first << ": Rs. " << toPair.second << endl;
            }
        }
    }
};

int main() {
    BankManagement bank;
    int choice;
    char op;

    do {
        system("cls");
        cout << "\t\t::Bank Management System" << endl;
        cout << "\t\t1. Create New Account" << endl;
        cout << "\t\t2. Show All Accounts" << endl;
        cout << "\t\t3. Search Account" << endl;
        cout << "\t\t4. Deposit Money" << endl;
        cout << "\t\t5. Withdraw Money" << endl;
        cout << "\t\t6. Delete Account" << endl;
        cout << "\t\t7. Transfer Money" << endl;
        cout << "\t\t8. Apply for Loan" << endl;
        cout << "\t\t9. Process Top Loan" << endl;
        cout << "\t\t10. Show Transaction Graph" << endl;
        cout << "\t\t11. Pay Loan Amount" << endl;
        cout << "\t\t12. Exit" << endl;
        cout << "\t\tEnter Your Choice : ";
        cin >> choice;

        switch (choice) {
        case 1: {
            string name;
            double balance;
            cout << "Enter Name: ";
            cin >> ws;
            getline(cin, name);
            cout << "Enter Initial Balance: ";
            cin >> balance;
            bank.AddAccount(name, balance);
            break;
        }
        case 2:
            bank.showAllAccounts();
            break;
        case 3: {
            int acc;
            cout << "Enter Account Number: ";
            cin >> acc;
            bank.searchAccount(acc);
            break;
        }
        case 4: {
            int acc;
            double amt;
            cout << "Enter Account Number: ";
            cin >> acc;
            cout << "Enter Amount to Deposit: ";
            cin >> amt;
            bank.deposit(acc, amt);
            break;
        }
        case 5: {
            int acc;
            double amt;
            cout << "Enter Account Number: ";
            cin >> acc;
            cout << "Enter Amount to Withdraw: ";
            cin >> amt;
            bank.withdraw(acc, amt);
            break;
        }
        case 6: {
            int acc;
            cout << "Enter Account Number to Delete: ";
            cin >> acc;
            bank.deleteAccount(acc);
            break;
        }
        case 7: {
            int from, to;
            double amt;
            cout << "Enter Sender Account Number: ";
            cin >> from;
            cout << "Enter Receiver Account Number: ";
            cin >> to;
            cout << "Enter Amount to Transfer: ";
            cin >> amt;
            bank.transfer(from, to, amt);
            break;
        }
        case 8: {
            int acc, priority;
            double amt;
            cout << "Enter Account Number: ";
            cin >> acc;
            cout << "Enter Loan Amount: ";
            cin >> amt;
            cout << "Enter Priority (higher is urgent): ";
            cin >> priority;
            bank.applyLoan(acc, amt, priority);
            break;
        }
        case 9:
            bank.processLoan();
            break;
        case 10:
            bank.showTransactionHistory();
            break;
        case 11: {
            int acc;
            double amt;
            cout << "Enter Account Number: ";
            cin >> acc;
            cout << "Enter Loan Payment Amount: ";
            cin >> amt;
            bank.payLoan(acc, amt);
            break;
        }
        case 12:
            exit(0);
        }

        cout << "\nDo you want to continue? (Y/N): ";
        cin >> op;

    } while (op == 'y' || op == 'Y');

    return 0;
}