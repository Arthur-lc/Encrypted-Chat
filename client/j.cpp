#include <iostream>
#include <fstream>
#include <nlohmann/json.hpp>

using namespace std;

using json = nlohmann::json;

void handle_key_exchange(const json& j) {
    cout << "[INITIATE_KEY_EXCHANGE]\n";
    cout << "Epoch ID: "    << j["payload"]["epoch_id"] << "\n";
    cout << "Group Size: "  << j["payload"]["groupSize"] << "\n";

    cout << "Ordered members:\n";
    for (auto& member : j["payload"]["orderedMembers"]) {
        cout << "  " << member["username"]
                  << " (publicKey = " << member["publicKey"] << ")\n";
    }
}

void handle_message(const json& j) {
    cout << "Sender: "      << j["payload"]["sender"] << "\n";
    cout << "Timestamp: "   << j["payload"]["timestamp"] << "\n";
    cout << "Ciphertext: "  << j["payload"]["ciphertext"] << "\n";
}

void process_json(const json& j) {
    try {
        string type = j.at("type");  // throws if "type" missing
        if (type == "INITIATE_KEY_EXCHANGE") {
            handle_key_exchange(j);
        } else if (type == "MESSAGE") {
            handle_message(j);
        } else {
            cout << "[UNKNOWN TYPE] " << type << "\n";
        }
    } catch (const exception& e) {
        cerr << "Invalid JSON structure: " << e.what() << "\n";
    }
}

/* int main() {
    ifstream file("jason.json");
    if (!file.is_open()) {
        cerr << "Could not open j.json\n";
        return 1;
    }

    try {
        json j;
        file >> j;  // parse JSON from file
        cout << "Type: " << j["type"];
        process_json(j);
    } catch (const exception& e) {
        cerr << "Failed to parse j.json: " << e.what() << "\n";
        return 1;
    }

    return 0;
}
 */