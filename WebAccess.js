import Net from 'net';
const Client = new Net.Socket();

const Port = 6942;
const Host = "127.0.0.1";
function RequestBalance() {
    let Address = Document.getElementById("BalanceLookupEntry").value;
    Client.connect({ port: Port, host: Host }, () => {
        Client.write("BaLook;" + Address);
    });

    Client.on('data', (data) => {
        document.getElementById("BalanceLookupFeedback").innerHTML = data.toString('utf-8');
    });
        
}