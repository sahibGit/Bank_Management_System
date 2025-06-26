DROP TABLE IF EXISTS accounts;

CREATE TABLE IF NOT EXISTS accounts (
    account_num INT AUTO_INCREMENT PRIMARY KEY,
    name VARCHAR(100) NOT NULL,
    balance DOUBLE DEFAULT 0
)AUTO_INCREMENT = 100001;

ALTER TABLE accounts DROP PRIMARY KEY;
ALTER TABLE accounts MODIFY COLUMN account_num INT NOT NULL AUTO_INCREMENT PRIMARY KEY;
CREATE TABLE IF NOT EXISTS transactions (
    id INT AUTO_INCREMENT PRIMARY KEY,
    from_account INT,
    to_account INT,
    amount DOUBLE,
    transfer_time TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    FOREIGN KEY (from_account) REFERENCES accounts(account_num) ON DELETE SET NULL,
    FOREIGN KEY (to_account) REFERENCES accounts(account_num) ON DELETE SET NULL
);

CREATE TABLE IF NOT EXISTS loans (
    id INT AUTO_INCREMENT PRIMARY KEY,
    account_num INT NOT NULL,
    original_amount DOUBLE NOT NULL,
    remaining_amount DOUBLE NOT NULL,
    priority INT NOT NULL,
    approved BOOLEAN DEFAULT FALSE,
    request_time TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    FOREIGN KEY (account_num) REFERENCES accounts(account_num) ON DELETE CASCADE
);
