using System;
using System.Collections.Generic;
using System.Data.SqlClient;
using System.Diagnostics;
using System.Linq;
using System.Web;
using GarageDoor.JsonObjects;

namespace GarageDoor
{
    public class Log
    {
        public static int LogDoorCommand
            (DoorCommands priorCommand, int startStatus, DoorCommands command, int endStatus, long commandDuration)
        {
            int rowsInserted = 0;
            if (!EnvironmentVariables.SqlLogging) return 0;

            string sql = "";
            using (SqlConnection conn = new SqlConnection(EnvironmentVariables.ConnString))
            using (SqlCommand sqlCommand = new SqlCommand())
            {
  
                    conn.Open();
                    sqlCommand.Connection = conn;
                    sql = 
                        $"INSERT INTO LogCommands VALUES(" +
                        $"'{DateTime.Now}', {(int)priorCommand}, '{((DoorCommands)priorCommand).ToString()}'," + 
                        $"{startStatus}, '{((StatusBits)startStatus).ToString()}'," +
                        $"{(int)command}, '{(command).ToString()}', {commandDuration}, " + 
                        $"{endStatus}, '{((StatusBits)endStatus).ToString()}')";
                    sqlCommand.CommandText = sql;
                    Debug.WriteLine(sql);
                    rowsInserted = sqlCommand.ExecuteNonQuery();
            }
            return rowsInserted;
        }

        public static int LogSensor(SensorReadingsForOperation readingsForOperation)
        {
            int rowsInserted = 0;
            string sql = "";
            using (SqlConnection conn = new SqlConnection(EnvironmentVariables.ConnString))
            using (SqlCommand sqlCommand = new SqlCommand())
            {
                conn.Open();
                sqlCommand.Connection = conn;

                foreach (SensorReading reading in readingsForOperation.Readings)
                {
                    sql = $"INSERT INTO LogSensor values ('{DateTime.Now}', '{reading.LogTime}', {reading.Rpm})";
                    sqlCommand.CommandText = sql;
                    rowsInserted += sqlCommand.ExecuteNonQuery();
                }
                Debug.WriteLine($"{nameof(rowsInserted)}={rowsInserted}");
            }
            return rowsInserted;
        }

    }
}