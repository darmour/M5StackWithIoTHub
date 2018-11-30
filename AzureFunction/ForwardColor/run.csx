using System;
using Newtonsoft.Json;
using Newtonsoft.Json.Linq;
//using System.Linq.Expressions;
using Microsoft.Azure.Devices;
using System.Text;


static ServiceClient serviceClient;

//Get the connection string from your IoT Hub's Shared Access Policies - use the IoTHubOwner primary key connection string
static string connectionString = "";  //TODO place your connection string in the quotes

private async static Task SendCloudToDeviceMessageAsync(string msg, string deviceID)
{
     string sendToDevice = "";
     var commandMessage = new 
      Message(Encoding.ASCII.GetBytes(msg));
      

      //Use your device IDs.  M5Stack1 and M5Stack2 are just sample device ID names
      if(deviceID=="M5Stack1")
      {
          sendToDevice = "M5Stack2";
      }
      else
      {
        sendToDevice = "M5Stack1";
      }
     await serviceClient.SendAsync(sendToDevice, commandMessage);
}

public static void Run(string myIoTHubMessage, ILogger log)
{
    JObject receivedMsg = JObject.Parse(myIoTHubMessage);
    string color = (string)receivedMsg["Color"];
    string deviceID = (string)receivedMsg["deviceId"];
       
    log.LogInformation($"C# IoT Hub trigger function processed a message: {myIoTHubMessage}");
    log.LogInformation($"the color is : {color}");
    log.LogInformation($"the device is : {deviceID}");

    serviceClient = ServiceClient.CreateFromConnectionString(connectionString);
    SendCloudToDeviceMessageAsync(color, deviceID).Wait();

}