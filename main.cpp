#include <iostream>
#include <fstream>
#include <string>
#include <sstream>
#include <sys/types.h>
#include <dirent.h>
#include <ctime>
#include <vector>

using namespace std;


class Transaction {
    public:
    string file_id;
    string ARN;
    int jdate; // data juliana

    void print(ostream &cout) const{
        cout << "\"" << this->file_id << "\",";
        cout << "\"" << this->ARN << "\",";
        cout << "\"2" << this->jdate << "\""; //Adding year digit
        cout << endl;
    }
    Transaction(){
        file_id = "";
        ARN = "";
        jdate = 0;
    }

    void clean(){
        ARN = "";
        jdate = 0;
    }
};

enum states {
    initial_state = 0,
    read_line_state,
    after_read_line_state,
    get_file_id_state,
    find_chargeback_state,
    get_arn_state,
    discard_01_state,
    get_date_state,
    error_state,
    close_state,
    new_file_state
};

vector <Transaction> allTransactions;
int count_different_than_27 = 0;

void parseReport(string &line, enum states &current_state, enum states &previous_state, Transaction &new_transaction){
    string data = "", file_id = "", ARN="", date="", intern_line="", key="", ddays="";
    int i_date = 0;
    stringstream ss(line);
    ss >> key; ss >> key;

    switch (current_state)
    {

    case initial_state:
        previous_state = initial_state;
        current_state = get_file_id_state;
        break;

    case get_file_id_state:
        data = line.substr(12,2);
        if(data != "90"){
            cout << "File ID line is invalid" << endl;
            previous_state = get_file_id_state;
            current_state = error_state;
            break;
        }
        file_id = line.substr(90,23);
        new_transaction.file_id = file_id;
        current_state = read_line_state;
        previous_state = get_file_id_state;
        break;

    case find_chargeback_state:
        data = line.substr(11+key.size(),2);
        if(data != "15") {
            current_state = read_line_state;
            previous_state = find_chargeback_state;
            break;
        }
        intern_line = line.substr(15+key.size(),2);
        if(intern_line != "00") {
            current_state = read_line_state;
            previous_state = find_chargeback_state;
            break;
        }
        previous_state = find_chargeback_state;
        current_state = get_arn_state;
        break;

    case get_arn_state:
        data = line.substr(11+key.size(),2);
        if(data != "15") {
            cout << "ARN line is invalid" << endl;
            current_state = error_state;
            previous_state = get_arn_state;
            break;
        }
        intern_line = line.substr(15+key.size(),2);
        if(intern_line != "00") {
            cout << "Interline isn't invalid" << endl;
            current_state = error_state;
            previous_state = get_arn_state;
            break;
        }

        ARN = line.substr(39+key.size(),23);
        new_transaction.ARN = ARN;
        previous_state = get_arn_state;
        current_state = read_line_state;
        break;
    
    case discard_01_state:
        data = line.substr(11+key.size(),2);
        if(data != "15") {
            cout << "Transaction type isn't 15" << endl;
            current_state = error_state;
            previous_state = get_date_state;
            break;
        }
        intern_line = line.substr(15+key.size(),2);
        if(intern_line != "01") {
            cout << "[01] - Interline isn't valid" << endl;
            current_state = error_state;
            previous_state = discard_01_state;
            break;
        }
        current_state = read_line_state;
        previous_state = discard_01_state;
        break;

    case get_date_state:
        data = line.substr(11+key.size(),2);
        if(data != "15") {
            cout << "Transaction type isn't 15" << endl;
            current_state = error_state;
            previous_state = get_date_state;
            break;
        }
        intern_line = line.substr(15+key.size(),2);
        if(intern_line != "02") {
            cout << "[02] - Interline isn't valid" << endl;
            current_state = error_state;
            previous_state = get_date_state;
            break;
        }

        ddays = line.substr(36+key.size(),2);
        if(ddays != "27"){
            count_different_than_27++;
            cout << "Warning dday different than 27" << endl;
        }
        date = line.substr(48+key.size(),4);
        i_date = stoi(date);
        new_transaction.jdate = i_date;

        previous_state = get_date_state;
        current_state = close_state;
        break;

    case read_line_state:
        if(previous_state == initial_state) current_state = get_file_id_state;
        if(previous_state == get_file_id_state) current_state = find_chargeback_state;
        if(previous_state == find_chargeback_state) current_state = find_chargeback_state;
        if(previous_state == get_arn_state) current_state = discard_01_state;
        if(previous_state == discard_01_state) current_state = get_date_state;
        if(previous_state == get_date_state) current_state = close_state;
        previous_state = read_line_state;
        break;

    case close_state:
        allTransactions.push_back(new_transaction);
        new_transaction.clean();
        current_state = read_line_state;
        previous_state = find_chargeback_state;
        break;

    case error_state:
        if(previous_state == get_file_id_state){
            cout << "file without file id state, going to the next file." << endl;
            current_state = new_file_state;
            previous_state = close_state;
            break;
        }
        current_state = find_chargeback_state;
        previous_state = close_state;
        break;
    default:
        break;
    }
}

void getInput(string path){
    DIR *dr;
    struct dirent *en;
    dr = opendir(path.c_str());
    if (dr) {
      while ((en = readdir(dr)) != NULL) {
        string filename(en->d_name);
        cout << "Reading File: " << filename << endl;
        ifstream fin;
        fin.open(path+'/'+filename);

        // Initialize parse
        enum states current_state = initial_state;
        enum states previous_state = initial_state;

        // Building Transaction
        Transaction new_transaction;

        string line;

        while(getline(fin, line)){
            while(not line.empty() && current_state != read_line_state ){
                if(current_state == after_read_line_state){
                    current_state = read_line_state;
                }
                parseReport(line, current_state, previous_state, new_transaction);
                if(current_state == new_file_state)
                    goto endfile;
            }
            if(current_state == read_line_state)
                current_state = after_read_line_state;
        }
    endfile:
        fin.close();
      }
      closedir(dr);
   } else {
    cout << "No reports founded." << endl;
   }
}

bool sortbyarn(Transaction &A, Transaction &B){
    return A.ARN <  B.ARN;
}

void outputSolutions(vector<Transaction> transactions, string outputFile){
    sort(transactions.begin(), transactions.end(), sortbyarn);

    ofstream myfile;
    myfile.open(outputFile);

    if(transactions.empty()){
        myfile << "There is no solution." << endl;
        return ;
    }
    if(count_different_than_27){
        cout << "Warning" << endl;
        cout << count_different_than_27 << " different than BR 27." <<endl;
    }
    myfile << "\"FILE_ID\", \"ARN\", \"Julian Date Time\"" << endl;

    for(int i=0; i < transactions.size(); i++){
        transactions[i].print(myfile);
    }

    myfile.close();
}

int main(int argc, char *argv[]){
    string path = "./reports";
    string outputFile = "output.csv";
    getInput(path);
    outputSolutions(allTransactions, outputFile);
    return 0;
}