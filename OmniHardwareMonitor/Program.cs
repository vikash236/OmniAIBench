using System;
using System.Collections.Generic;
using System.Text.Json;
using LibreHardwareMonitor.Hardware;

namespace OmniHardwareMonitor
{
    // Visitor to update all hardware sensors
    public class UpdateVisitor : IVisitor
    {
        public void VisitComputer(IComputer computer)
        {
            computer.Traverse(this);
        }

        public void VisitHardware(IHardware hardware)
        {
            hardware.Update();
            foreach (IHardware subHardware in hardware.SubHardware)
                subHardware.Accept(this);
        }

        public void VisitSensor(ISensor sensor) { }
        public void VisitParameter(IParameter parameter) { }
    }

    // Sensor data model for JSON output
    public class SensorData
    {
        public string Hardware { get; set; } = "";
        public string Type { get; set; } = "";
        public string Sensor { get; set; } = "";
        public float Value { get; set; }
        public string Unit { get; set; } = "";
        public string SensorType { get; set; } = "";
    }

    class Program
    {
        static void Main(string[] args)
        {
            // Initialize computer with all sensors enabled
            Computer computer = new Computer
            {
                IsCpuEnabled = true,
                IsGpuEnabled = true,
                IsMemoryEnabled = true,
                IsMotherboardEnabled = true,
                IsControllerEnabled = true,
                IsStorageEnabled = true,
                IsNetworkEnabled = true
            };

            computer.Open();

            var updateVisitor = new UpdateVisitor();

            // Continuous monitoring loop
            while (true)
            {
                try
                {
                    // Update all sensors
                    computer.Accept(updateVisitor);

                    var sensorList = new List<SensorData>();

                    // Iterate through all hardware
                    foreach (IHardware hardware in computer.Hardware)
                    {
                        CollectSensors(hardware, sensorList);
                    }

                    // Serialize to minified JSON
                    var json = JsonSerializer.Serialize(sensorList, new JsonSerializerOptions
                    {
                        WriteIndented = false
                    });

                    // Output to stdout
                    Console.WriteLine(json);

                    // Wait 1 second before next update
                    Thread.Sleep(1000);
                }
                catch (Exception ex)
                {
                    // Silent error handling - write to stderr to avoid breaking JSON output
                    Console.Error.WriteLine($"Error: {ex.Message}");
                    Thread.Sleep(1000);
                }
            }
        }

        static void CollectSensors(IHardware hardware, List<SensorData> sensorList)
        {
            // Collect sensors from this hardware
            foreach (ISensor sensor in hardware.Sensors)
            {
                if (sensor.Value.HasValue)
                {
                    sensorList.Add(new SensorData
                    {
                        Hardware = hardware.Name,
                        Type = hardware.HardwareType.ToString(),
                        Sensor = sensor.Name,
                        Value = sensor.Value.Value,
                        Unit = GetSensorUnit(sensor.SensorType),
                        SensorType = sensor.SensorType.ToString()
                    });
                }
            }

            // Recursively collect from sub-hardware
            foreach (IHardware subHardware in hardware.SubHardware)
            {
                CollectSensors(subHardware, sensorList);
            }
        }

        static string GetSensorUnit(SensorType sensorType)
        {
            return sensorType switch
            {
                SensorType.Voltage => "V",
                SensorType.Current => "A",
                SensorType.Clock => "MHz",
                SensorType.Temperature => "°C",
                SensorType.Load => "%",
                SensorType.Frequency => "MHz",
                SensorType.Fan => "RPM",
                SensorType.Flow => "L/h",
                SensorType.Control => "%",
                SensorType.Level => "%",
                SensorType.Factor => "1",
                SensorType.Power => "W",
                SensorType.Data => "GB",
                SensorType.SmallData => "MB",
                SensorType.Throughput => "MB/s",
                SensorType.TimeSpan => "s",
                SensorType.Energy => "mWh",
                _ => ""
            };
        }
    }
}
