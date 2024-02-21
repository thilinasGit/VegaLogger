<?php
declare(strict_types=1);

require 'C:\xampp\htdocs\mqtt_data\vendor\autoload.php';
// require  '/../shared/config.php';

// use PhpMqtt\Client\Examples\Shared\SimpleLogger;
// use PhpMqtt\Client\Exceptions\MqttClientException;
use PhpMqtt\Client\MqttClient;
// use Psr\Log\LogLevel;

// Create an instance of a PSR-3 compliant logger. For this example, we will also use the logger to log exceptions.
// $logger = new SimpleLogger(LogLevel::INFO);

include('config.php');

$connection = new mysqli($host, $user, $password, $database);
// Check connection
if ($connection->connect_error) {
    die("Connection failed: " . $connection->connect_error);
}


try {
    
    $MQTT_BROKER_HOST='broker.mqtt-dashboard.com';
    $MQTT_BROKER_PORT=1883;
    // Create a new instance of an MQTT client and configure it to use the shared broker host and port.
    $client = new MqttClient($MQTT_BROKER_HOST, $MQTT_BROKER_PORT);

    // Connect to the broker without specific connection settings but with a clean session.
    $client->connect(null, true);
    

    // Subscribe to the topic 'foo/bar/baz' using QoS 0.
    $client->subscribe('TopicHidden', function (string $topic, string $message, bool $retained) use ($connection,$client) {
        // Assuming the MQTT message contains JSON data
        $query="INSERT INTO log (ID,A1,V1,E1,P1,S1,T1,A2,V2,E2,P2,S2,T2,A3,V3,E3,P3,S3,T3,ax,ay,az,X,Y,Z,mx,my,mz,timestamp) VALUES (?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?)";
        $stmt=$connection->prepare($query);

        $data_in = json_decode($message, true);
       
        if ($stmt) {
            $stmt->bind_param('sssssssssssssssssssssssssssss',$data_in['ID'],$data_in['A1'],$data_in['V1'],$data_in['E1'],$data_in['P1'],$data_in['S1'],$data_in['T1'],$data_in['A2'],$data_in['V2'],$data_in['E2'],$data_in['P2'],$data_in['S2'],$data_in['T2'],$data_in['A3'],$data_in['V3'],$data_in['E3'],$data_in['P3'],$data_in['S3'],$data_in['T3'],$data_in['ax'],$data_in['ay'],$data_in['az'],$data_in['X'],$data_in['Y'],$data_in['Z'],$data_in['mx'],$data_in['my'],$data_in['mz'],"0");
            if ($stmt->execute()) {
                echo "Data inserted successfully!";
            } else {
                echo "Error inserting data: " . $stmt->error;
            }
            $stmt->close();
        } else {
            echo "Error preparing statement: " . $connection->error;
        }
        // $stmt->bind_param('sssssssssssssssssssssssssssss',$data_in['ID'],$data_in['A1'],$data_in['V1'],$data_in['E1'],$data_in['P1'],$data_in['S1'],$data_in['T1'],$data_in['A2'],$data_in['V2'],$data_in['E2'],$data_in['P2'],$data_in['S2'],$data_in['T2'],$data_in['A3'],$data_in['V3'],$data_in['E3'],$data_in['P3'],$data_in['S3'],$data_in['T3'],$data_in['ax'],$data_in['ay'],$data_in['az'],$data_in['X'],$data_in['Y'],$data_in['Z'],$data_in['mx'],$data_in['my'],$data_in['mz'],0);
        if($message){
            echo sprintf('We received a %s on topic [%s]: %s', $retained ? 'retained message' : 'message',$topic,$message);
        }
        


        // After receiving the first message on the subscribed topic, we want the client to stop listening for messages.
        // $client->interrupt();
    }, 0);

    // Since subscribing requires to wait for messages, we need to start the client loop which takes care of receiving,
    // parsing and delivering messages to the registered callbacks. The loop will run indefinitely, until a message
    // is received, which will interrupt the loop.
    $client->loop(true);

    // Gracefully terminate the connection to the broker.
    $client->disconnect();
} catch (MqttClientException $e) {
    // MqttClientException is the base exception of all exceptions in the library. Catching it will catch all MQTT related exceptions.
    echo sprint('Subscribing to a topic using QoS 0 failed. An exception occurred.', ['exception' => $e]);
}

?>
