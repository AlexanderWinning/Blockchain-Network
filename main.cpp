#include<iostream>
#include<fstream>
#include<cstring>
#include<list>
#include<string>
#include<vector>
#include<stdc++.hpp>
#include<boost/algorithm/string.hpp>
#include<jsoncpp/include/json/json.h>
#include"sha256.h" // Not the best look into it further
#include<WinSock2.h>
#include<Windows.h>
#include<WS2tcpip.h>

#pragma comment(lib, "ws2_32.lib")

#define DefaultBufLen 1024
#define DefaultPort 6942

using namespace std;

ifstream ifs("Blockchain.json");
Json::Value Blockchain;
Json::Reader RBlockchain;
ifstream ifs("Accounts.json");
Json::Value Accounts;
Json::Reader RAccounts;

SOCKET Socket = INVALID_SOCKET;
int difficulty = 0;

WSADATA wsaData;
struct addrinfo* result = NULL,
    * ptr = NULL,
    hints;
struct sockaddr_in Service;

int recvbuflen = DefaultBufLen;
char SendBuf[DefaultBufLen] = "";
char RecvBuf[DefaultBufLen] = "";

vector<string> MemPool;

int main() {
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != NO_ERROR) {
        wprintf(L"Startup failed with error code %d\n", WSAGetLastError());
        return 1;
    };

    Socket = socket(AF_INET, SOCK_DGRAM, IPPROTO_TCP);
    if (Socket == INVALID_SOCKET) {
        wprintf(L"Socket failed with error: %1d\n", WSAGetLastError());
        WSACleanup();
        return 1;
    };

    Service.sin_family = AF_INET;
    Service.sin_addr.s_addr = inet_addr("127.0.0.1");
    Service.sin_port = htons(DefaultPort);

    bind(Socket, (SOCKADDR*)&Service, sizeof(Service));

    while (1) {
        if (listen(Socket, 1) == SOCKET_ERROR) {
            wprintf(L"listen failed with error: %ld\n", WSAGetLastError());
            closesocket(Socket);
            WSACleanup();
            return 1;
        }

        SOCKET AcceptingSocket;
        wprintf(L"Waiting for client to connect...\n");

        AcceptingSocket = accept(Socket, NULL, NULL);
        if (AcceptingSocket == INVALID_SOCKET) {
            wprintf(L"accept failed with error: %ld\n", WSAGetLastError());
            closesocket(Socket);
            WSACleanup();
            return 1;
        }
        else {
            wprintf(L"Client connected.\n");
        }

        int Error = 0;

        do {

            Error = recv(Socket, RecvBuf, recvbuflen, 0);
            if (Error > 0)
                printf("Bytes received: %d\n", Error);
            else if (Error == 0)
                printf("Connection closed\n");
            else
                printf("recv failed: %d\n", WSAGetLastError());

        } while (Error > 0);

        string TranslatedRecvBuf = RecvBuf;

        vector<string> Request;
        boost::split(Request, TranslatedRecvBuf, boost::is_any_of(";"));
        if (Request[0] == "RBlock") {
            LoadBlock(Request[1]);
            AppendToBuf();
            if (send(Socket, SendBuf, (int)strlen(SendBuf), 0) == SOCKET_ERROR) {
                wprintf(L"Send failed with error %d\n", WSAGetLastError());
            };
        }
        else if (Request[0] == "DBlock") {
            if (stoi(Request[1]) != Blockchain.size()) {
                if (stoi(Request[1]) > Blockchain.size()) {
                    int TempBlockHeight = (Blockchain.size() - 1);
                    for (int BlocksAway = 1; BlocksAway < ((stoi(Request[1]) + 1) - TempBlockHeight); BlocksAway++) {
                        string Plaintext = "RBlock;" + to_string(BlocksAway) + ";";
                        strcpy(SendBuf, Plaintext.c_str());
                        if (send(Socket, SendBuf, (int)strlen(SendBuf), 0) == SOCKET_ERROR) {
                            wprintf(L"Send failed with error %d\n", WSAGetLastError());
                        };
                        do {
                            Error = recv(Socket, RecvBuf, recvbuflen, 0);
                            if (Error > 0)
                                printf("Bytes received: %d\n", Error);
                            else if (Error == 0)
                                printf("Connection closed\n");
                            else
                                printf("recv failed: %d\n", WSAGetLastError());

                        } while (Error > 0);
                        AppendToStruct(Request);
                        ValidateBlock();
                    };
                };
            }
            else {
                AppendToStruct(Request);
                if (ValidateBlock()) {
                    for (int Count = 0; Count < Accounts["KnownNodes"].size(); Count++) {
                        DistributeInfo(RecvBuf, Accounts["KownNodes"][Count].asCString());
                    };
                };
            }
        }
        else if (Request[0] == "TxIncl") {
            if (ValidateTx(Request[1]) && NotInMemPool(Request[1])) {
                MemPool.push_back(Request[1]);
                for (int Count = 0; Count < Accounts["KnownNodes"].size(); Count++) {
                    DistributeInfo(RecvBuf, Accounts["KownNodes"][Count].asCString());
                };
            };
        }
        else if (Request[0] == "BaLook") {
            string Plaintext = to_string(LookupBalance(Request[1]));
            strcpy(SendBuf, Plaintext.c_str());
            if (send(Socket, SendBuf, (int)strlen(SendBuf), 0) == SOCKET_ERROR) {
                wprintf(L"Send failed with error %d\n", WSAGetLastError());
            };
        };

        closesocket(Socket);

        WSACleanup();
    };
};

bool NotInMemPool(string Tx) {
    for (int i = 1; i < MemPool.size(); i++) {
        if (MemPool[i] == Tx) {
            return false;
        };
    };
    return true;
};

bool LoadBlock(string BlockHeight) {
	if (stoi(BlockHeight) > Blockchain.size()) {
		return false;
	};
	Block.BlockHash = (Blockchain[BlockHeight]["BlockHash"]).asString();
	Block.BlockSize = (Blockchain[BlockHeight]["BlockSize"]).asInt();
	BlockHeader.Version = (Blockchain[BlockHeight]["BlockHeader"]["Version"]).asInt();
	BlockHeader.PreviousBlockHash = Blockchain[BlockHeight]["BlockHeader"]["PreviousBlockHash"].asString();
	BlockHeader.MerkleRoot = Blockchain[BlockHeight]["BlockHeader"]["MerkleRoot"].asString();
	BlockHeader.Timestamp = Blockchain[BlockHeight]["BlockHeader"]["Timestamp"].asString();
    BlockHeader.TargetDifficulty = Blockchain[BlockHeight]["BlockHeader"]["DifficultyTarget"].asInt();
    BlockHeader.Nonce = Blockchain[BlockHeight]["BlockHeader"]["Nonce"].asInt();
    Block.TransactionCounter = Blockchain[BlockHeight]["TransactionCount"].asInt();
    for (int i = 0; i < Block.TransactionCounter; i++) {
        Block.Transactions.push_back(Blockchain[BlockHeight]["Transactions"][i].asString());
    };
	return true;
};

void AppendToBuf() {
    string Plaintext = Block.BlockHash + ";" + to_string(Block.BlockSize) + ";" + to_string(BlockHeader.Version) + ";" + BlockHeader.PreviousBlockHash + ";" + BlockHeader.MerkleRoot + ";" + BlockHeader.Timestamp + ";" + to_string(BlockHeader.Nonce) + ";" + to_string(Block.TransactionCounter);
    for (int i = 0; i < Block.Transactions.size(); i++) {
        Plaintext += ";" + Block.Transactions[i];
    };
    strcpy(SendBuf, Plaintext.c_str());
}

void AppendToStruct(vector<string> Request) {
    Block.BlockHeight = stoi(Request[1]);
    Block.BlockHash = Request[2];
    Block.BlockSize = stoi(Request[3]);
    BlockHeader.Version = stoi(Request[4]);
    BlockHeader.PreviousBlockHash = Request[5];
    BlockHeader.MerkleRoot = Request[6];
    BlockHeader.Timestamp = Request[7];
    BlockHeader.TargetDifficulty = stoi(Request[8]);
    BlockHeader.Nonce = stoi(Request[9]);
    Block.TransactionCounter = stoi(Request[10]);
    vector<string> RTransactions;
    for (int RCount = 11; RCount < Request.size(); RCount++) {
        RTransactions.push_back(Request[RCount]);
    };
    Block.Transactions = RTransactions;
};

void DistributeInfo(char Buf[DefaultBufLen], const char* IPAddress) { // need to add block to the send buffer
    Service.sin_family = AF_INET;
    Service.sin_addr.s_addr = inet_addr(IPAddress);
    Service.sin_port = htons(DefaultPort);


    if (connect(Socket, (SOCKADDR*)&Socket, sizeof(Socket)) == SOCKET_ERROR) {
        wprintf(L"Connect failed with error: %d\n", WSAGetLastError());
        return;
    };

    if (send(Socket, Buf, (int)strlen(Buf), 0) == SOCKET_ERROR) {
        wprintf(L"Send failed with error %d\n", WSAGetLastError());
        return;
    };

    int Error = 0;

    do {
        Error = recv(Socket, RecvBuf, recvbuflen, 0);
        if (Error > 0) {
            wprintf(L"Bytes received: %d\n", Error);
        }
        else if (Error == 0) {
            wprintf(L"Connection closed\n");
        }
        else {
            wprintf(L"recv failed with error: %d\n", WSAGetLastError());
        }
    } while (Error > 0);

    if (shutdown(Socket, SD_SEND) == SOCKET_ERROR) {
        wprintf(L"shutdown failed with error: %d\n", WSAGetLastError());
        closesocket(Socket);
        WSACleanup();
    };

    if (closesocket(Socket) == SOCKET_ERROR) {
        wprintf(L"close failed with error: %d\n", WSAGetLastError());
        WSACleanup();
    };

    WSACleanup();
};

struct BlockHeaderStandard{
    int Version;
    string PreviousBlockHash;
    string MerkleRoot;
    string Timestamp;
    int TargetDifficulty;
    int Nonce;
}BlockHeader;

struct BlockStandard{
    int BlockHeight;
    string BlockHash;
    int BlockSize;
    struct BlockHeader;
    int TransactionCounter;
    vector<string> Transactions;
}Block;

string DecryptSignature(string Signature, string PublicKey) {
    return "Signature";// waiting for better sha algo 
};

string MerkleRoot() {
    string TxSignatures;
    for (int Txs = 0; Txs == Block.Transactions.size(); Txs++) {
        vector<string> TxSig;
        boost::split(TxSig, Block.Transactions[Txs], boost::is_any_of(";"));
        TxSignatures += TxSig[4];
    };
    return sha256(sha256(Blockchain[Blockchain.size()-1][2][2].asString() + TxSignatures));
};

int LookupBalance(string Account){ 
    return Accounts[Account][0].asInt();
};

int LookupTxCount(string Account){
    return Accounts[Account][1].asInt();
};

void WriteBlock(string Transactions) {
    string text = "{" + to_string(Block.BlockHeight) + ": [{ \"BlockHash\": \"" + Block.BlockHash + "\" }, { \"BlockSize\": " + to_string(Block.BlockSize) + " }, { \"BlockHeader\" : [{ \"Version\": " + to_string(BlockHeader.Version) + " },{ \"PreviousBlockHash\": \"" + BlockHeader.PreviousBlockHash + "\" }, { \"MerkleRoot\": \"" + BlockHeader.MerkleRoot + "\" }, { \"Timestamp\": \"" + BlockHeader.Timestamp + "\" },{ \"DifficultyTarget\": \"" + to_string(BlockHeader.TargetDifficulty) + "\" }, { \"Nonce\": \"" + to_string(BlockHeader.Nonce) + "\" }] }, { \"TransactionCounter\": " + to_string(Block.TransactionCounter) + " }, { \"Transactions\": [" + Transactions + "] }]}";
    bool parsingSuccessful = RBlockchain.parse(text, Blockchain);
    if (!parsingSuccessful)
    {
        cout << "Error parsing the string" << endl;
    }

    for (Json::Value::const_iterator outer = Blockchain.begin(); outer != Blockchain.end(); outer++)
    {
        for (Json::Value::const_iterator inner = (*outer).begin(); inner != (*outer).end(); inner++)
        {
            cout << inner.key() << ": " << *inner << endl;
        }
    }
};

bool ValidateTx(string TransactionIn) {
    vector<string> Tx;
    boost::split(Tx, TransactionIn, boost::is_any_of(":"));
    string Plaintext = Tx[0] + Tx[1] + Tx[2] + Tx[3];
    if (DecryptSignature(Tx[4], Tx[0]) != Plaintext) {
        return false;
    };
    if (LookupBalance(Tx[0]) < stoi(Tx[1])) {
        return false;
    };
    if (LookupTxCount(Tx[0]) != stoi(Tx[3])) {
        return false;
    };
    return true;
}

void ExecuteTx() {
    for (int i = 0; i < Block.TransactionCounter; i++) {
        vector<string> Tx;
        boost::split(Tx, Block.Transactions[i], boost::is_any_of(":"));
        if (i == 0) {
            Accounts[Tx[2]][0] = Accounts[Tx[2]][0].asInt() + stoi(Tx[1]);
        }
        else {
            Accounts[Tx[0]][0] = Accounts[Tx[0]][0].asInt() - stoi(Tx[1]);
            Accounts[Tx[2]][0] = Accounts[Tx[2]][0].asInt() + stoi(Tx[1]);
            Accounts[Tx[0]][1] = Accounts[Tx[0]][1].asInt() + 1;
        }
    }
}

void ScrubMemPool() {
    for (int i = 1; i < (Block.TransactionCounter - 1); i++) {
        MemPool.erase(remove(MemPool.begin(), MemPool.end(), Block.Transactions[i]), MemPool.end());
    }
}

bool ValidateBlock(){
    if (sha256(to_string(BlockHeader.Version) + BlockHeader.PreviousBlockHash + BlockHeader.MerkleRoot + BlockHeader.Timestamp + to_string(BlockHeader.Nonce)) != Block.BlockHash) {
        return false;
    };
    if (BlockHeader.Version < Accounts["CurrentWnIP"][0].asInt()) {
        return false;
    };
    if (BlockHeader.MerkleRoot != MerkleRoot()){
        return false;
    };
    if (difficulty == 0) {
        difficulty = BlockHeader.TargetDifficulty;
    };
    if (BlockHeader.PreviousBlockHash != Blockchain[Blockchain.size() - 1][0].asString()) {
        return false;
    };
    if (Block.TransactionCounter != Block.Transactions.size()) {
        return false;
    };
    int DiffCount = 0;
    for (int BlockHashIndex = 0; BlockHashIndex < Block.BlockHash.size(); BlockHashIndex++) {
        if(Block.BlockHash[BlockHashIndex] != '0'){
            break;
        };
        DiffCount++;
    };
    if (DiffCount < difficulty){
        return false;
    };
    vector<string> CoinbaseTx;
    boost::split(CoinbaseTx, Block.Transactions[0], boost::is_any_of(":"));
    if (stoi(CoinbaseTx[1]) != Accounts["CurrentWnIP"][1].asInt()) {
        return false;
    };
    if (CoinbaseTx[0] != "0x") {
        return false;
    };
    string Transactions;
    for (int TxNumber = 1; TxNumber < (Block.TransactionCounter - 1); TxNumber++){ //Skip coinbase tx Transaction[0] as will not validate due to minting of coins coming from a 0 address
        if (TxNumber == (Block.TransactionCounter - 2)) {
            Transactions += "\"" + Block.Transactions[TxNumber] + "\"";
        }
        else {
            Transactions += "\"" + Block.Transactions[TxNumber] + "\", ";
        }
        if (!ValidateTx(Block.Transactions[TxNumber])) {
            return false;
        };
    };
    ExecuteTx();
    ScrubMemPool();
    WriteBlock(Transactions);
    return true;
};