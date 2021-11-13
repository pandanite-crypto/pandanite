#include "../core/user.hpp"
#include "../core/api.hpp"
#include "../core/helpers.hpp"
#include "../core/common.hpp"
#include <map>
#include <iostream>
using namespace std;



int main() {
    map<string,User*> users;
    string host = "http://localhost:3000";

    while(true) {
        int x; 
        cout << "Choose an option: " <<endl;
        cout << "1: add a new user" <<endl;
        cout << "2: send money" <<endl;
        cin >> x;
        if (x == 1) {
            cout<<"Enter a nickname: (no spaces, case sensitive)"<<endl;
            string name;
            cin>>name;
            cout<<"Do you have a key file?"<<endl;
            int x;
            cout << "1: YES" <<endl;
            cout << "2: NO" <<endl;
            cin >> x;
            if (x == 1) {
                cout<<"Please enter the file name"<<endl;
                string filepath;
                cin>>filepath;
                json data = readJsonFromFile(filepath);
                users[name] = new User(data);
            } else {
                users[name] = new User();
                writeJsonToFile(users[name]->toJson(),"./keys/" + name + ".json");
            }
        } else {
            cout<<"Enter from user nickname:"<<endl;
            string from;
            cin>>from;
            cout<<"Enter to user nickname:"<<endl;
            string to;
            cin>> to;
            cout<<"Enter the amount:"<<endl;
            double amount;
            cin>>amount;
            User * fromUser = users[from];
            User * toUser = users[to];
            int blockId = getCurrentBlockCount(host) + 2; // usually send 1 block into the future
            cout<<"Creating transaction, current block: "<<blockId<<endl;
            Transaction t = fromUser->send(*toUser, amount, blockId);
            cout<<sendTransaction(host, t)<<endl;
        }
    }
}