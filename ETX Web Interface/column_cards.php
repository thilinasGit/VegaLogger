<?php 
include('config.php');

$connection = new mysqli($host, $user, $password, $database);
// Check connection
if ($connection->connect_error) {
    die("Connection failed: " . $connection->connect_error);
}

$id=$_GET['etx_id'];
//write query for all data
$sql="SELECT * FROM log WHERE ID ='$id' ORDER BY timestamp DESC LIMIT 1";
//make query &get result
$result=mysqli_query($connection,$sql);

//fetch the resulting rows as an array
$data=mysqli_fetch_all($result,MYSQLI_ASSOC);
?>
<?php 
if ($result->num_rows> 0) {
    foreach ($result as $row) {
        echo" <h4> last updated at ".$row['timestamp'] . "</h4>";
    }
}
?>
<div class="cardcolumn">
    <table class="cardnames">
        <?php
        echo "</br>";
        echo "</br>";
        echo "</br>";
        echo "</br>";
        echo "<tr>";
        echo "<td>Current</td>";
        echo "</tr>";
        echo "<tr>";
        echo "<td>Voltage</td>";
        echo "</tr>";
        echo "<tr>";
        echo "<td>Energy</td>";
        echo "</tr>";
        echo "<tr>";
        echo "<td>Power</td>";
        echo "</tr>";
        echo "<tr>";
        echo "<td>SOC</td>";
        echo "</tr>";
        echo "<tr>";
        echo "<td>Temperature</td>";
        echo "</tr>";
        ?>
    </table>
</div>
<div class="cardcolumn">
    <div class="card">
        <table>
        <?php
        // Step 4: Loop through the data and display it in the HTML table
        if ($result->num_rows> 0) {
            foreach ($result as $row) {
                echo "<h4>Solar</h4>" ;
                echo "<tr>";
                echo "<td>" . $row['A1'] . "</td>";
                echo "</tr>";
                echo "<tr>";
                echo "<td>" . $row['V1'] . "</td>";
                echo "</tr>";
                echo "<tr>";
                echo "<td>" . $row['E1'] . "</td>";
                echo "</tr>";
                echo "<tr>";
                echo "<td>" . $row['P1'] . "</td>";
                echo "</tr>";
                echo "<tr>";
                echo "<td>" . $row['S1'] . "</td>";
                echo "</tr>";
                echo "<tr>";
                echo "<td>" . $row['T1'] . "</td>";
                echo "</tr>";
            }
        } else {
        echo "<tr><td colspan='4'>No data available</td></tr>";
        }
        ?>
        </table>
    </div>
</div>
<div class="cardcolumn">
    <div class="card">

        <table>
        <?php
        // Step 4: Loop through the data and display it in the HTML table
        if ($result->num_rows> 0) {
            foreach ($result as $row) {
                echo "<h4>MPPT</h4>" ;
                echo "<tr>";
                echo "<td>" . $row['A2'] . "</td>";
                echo "</tr>";
                echo "<tr>";
                echo "<td>" . $row['V2'] . "</td>";
                echo "</tr>";
                echo "<tr>";
                echo "<td>" . $row['E2'] . "</td>";
                echo "</tr>";
                echo "<tr>";
                echo "<td>" . $row['P2'] . "</td>";
                echo "</tr>";
                echo "<tr>";
                echo "<td>" . $row['S2'] . "</td>";
                echo "</tr>";
                echo "<tr>";
                echo "<td>" . $row['T2'] . "</td>";
                echo "</tr>";
            }
        } else {
        echo "<tr><td colspan='4'>No data available</td></tr>";
        }
        ?>
        </table>
    </div>
</div>
<div class="cardcolumn">
    <div class="card">

        <table>
        <?php
        // Step 4: Loop through the data and display it in the HTML table
        if ($result->num_rows> 0) {
            foreach ($result as $row) {
                echo "<h4>Battery</h4>" ;
                echo "<tr>";
                echo "<td>" . $row['A3'] . "</td>";
                echo "</tr>";
                echo "<tr>";
                echo "<td>" . $row['V3'] . "</td>";
                echo "</tr>";
                echo "<tr>";
                echo "<td>" . $row['E3'] . "</td>";
                echo "</tr>";
                echo "<tr>";
                echo "<td>" . $row['P3'] . "</td>";
                echo "</tr>";
                echo "<tr>";
                echo "<td>" . $row['S3'] . "</td>";
                echo "</tr>";
                echo "<tr>";
                echo "<td>" . $row['T3'] . "</td>";
                echo "</tr>";
            }
        } else {
        echo "<tr><td colspan='4'>No data available</td></tr>";
        }
        ?>
        </table>
    </div>
</div>
<div class="cardcolumn">
    <table class="cardnames">
        <?php
        echo "</br>";
        echo "</br>";
        echo "</br>";
        echo "</br>";
        echo "<tr>";
        echo "<td>A</td>";
        echo "</tr>";
        echo "<tr>";
        echo "<td>V</td>";
        echo "</tr>";
        echo "<tr>";
        echo "<td>kWh</td>";
        echo "</tr>";
        echo "<tr>";
        echo "<td>W</td>";
        echo "</tr>";
        echo "<tr>";
        echo "<td>%</td>";
        echo "</tr>";
        echo "<tr>";
        echo "<td>°C</td>";
        echo "</tr>";
        ?>
    </table>
</div>


