<?php 
include('config.php');

$connection = new mysqli($host, $user, $password, $database);
// Check connection
if ($connection->connect_error) {
    die("Connection failed: " . $connection->connect_error);
}

$id=$_GET['etx_id'];

//write query for all data
$sql="SELECT ax,ay,az,X,Y,Z FROM log WHERE ID ='$id' ORDER BY timestamp DESC LIMIT 1";
//make query &get result
$result=mysqli_query($connection,$sql);

//fetch the resulting rows as an array
$data=mysqli_fetch_all($result,MYSQLI_ASSOC);
?>

<div style="width: 800px; margin:auto;" class="cardrow">
    <div class="anglecardcolumn">
        <div class="card">
        <?php
        // Step 4: Loop through the data and display it in the HTML table
        if ($result->num_rows> 0) {
            foreach ($result as $row) {
                echo "<h4>ax</h4>" ;
                echo "<tr>";
                echo "<td>" . $row['ax'] . "</td>";
                echo "</tr>";
            }
        } else {
            echo "<tr><td colspan='4'>No data available</td></tr>";
        }
        ?>
        </div>
    </div>
    <div class="anglecardcolumn">
        <div class="card">
        <?php
        // Step 4: Loop through the data and display it in the HTML table
        if ($result->num_rows> 0) {
            foreach ($result as $row) {
                echo "<h4>ay</h4>" ;
                echo "<tr>";
                echo "<td>" . $row['ay'] . "</td>";
                echo "</tr>";
            }
        } else {
            echo "<tr><td colspan='4'>No data available</td></tr>";
        }
        ?>
        </div>
    </div>
    <div class="anglecardcolumn">
        <div class="card">
        <?php
        // Step 4: Loop through the data and display it in the HTML table
        if ($result->num_rows> 0) {
            foreach ($result as $row) {
                echo "<h4>az</h4>" ;
                echo "<tr>";
                echo "<td>" . $row['az'] . "</td>";
                echo "</tr>";
            }
        } else {
            echo "<tr><td colspan='4'>No data available</td></tr>";
        }
        ?>
        </div>
    </div>
    <div class="anglecardcolumn">
        <div class="card">
        <?php
        // Step 4: Loop through the data and display it in the HTML table
        if ($result->num_rows> 0) {
            foreach ($result as $row) {
                echo "<h4>X</h4>" ;
                echo "<tr>";
                echo "<td>" . $row['X'] . "</td>";
                echo "</tr>";
            }
        } else {
            echo "<tr><td colspan='4'>No data available</td></tr>";
        }
        ?>
        </div>
    </div>
    <div class="anglecardcolumn">
        <div class="card">
        <?php
        // Step 4: Loop through the data and display it in the HTML table
        if ($result->num_rows> 0) {
            foreach ($result as $row) {
                echo "<h4>Y</h4>" ;
                echo "<tr>";
                echo "<td>" . $row['Y'] . "</td>";
                echo "</tr>";
            }
        } else {
            echo "<tr><td colspan='4'>No data available</td></tr>";
        }
        ?>
        </div>
    </div>
    <div class="anglecardcolumn">
        <div class="card">
        <?php
        // Step 4: Loop through the data and display it in the HTML table
        if ($result->num_rows> 0) {
            foreach ($result as $row) {
                echo "<h4>Z</h4>" ;
                echo "<tr>";
                echo "<td>" . $row['Z'] . "</td>";
                echo "</tr>";
            }
        } else {
            echo "<tr><td colspan='4'>No data available</td></tr>";
        }
        ?>
        </div>
    </div>
</div>