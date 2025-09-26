using System;
using System.IO;
using UnityEngine;

namespace Visualizer
{
    public class Initialize : MonoBehaviour
    {
        // Main
        public const string VERSION = "0.0.1";

        // Prefabs
        public GameObject selfCarPrefab; // self
        public GameObject CarPrefab;
        public GameObject TruckPrefab;
        public GameObject TrailerPrefab;
        public GameObject BusPrefab;
        public GameObject ConstructionVehiclePrefab;
        public GameObject BicyclePrefab;
        public GameObject MotorcyclePrefab;
        public GameObject PedestrianPrefab;
        public GameObject TrafficConePrefab;
        public GameObject BarrierPrefab;

        // Methods
        private Model.ModelCollection models;
        private Model.Plotter plotter;
        private Networking.UDPClient udpClient;

        void Start()
        {
            if (!Application.isPlaying)
                return;

            Transform scene = gameObject.transform;

            GameObject[] allModels = {
                selfCarPrefab, CarPrefab, TruckPrefab, TrailerPrefab,
                BusPrefab, ConstructionVehiclePrefab, BicyclePrefab, MotorcyclePrefab,
                PedestrianPrefab, TrafficConePrefab, BarrierPrefab
            };

            models = new(allModels);
            plotter = new(models, scene);

            Instantiate(selfCarPrefab, scene);

            udpClient = gameObject.AddComponent<Networking.UDPClient>();
            udpClient.onMessageReceived += onDetectionDataReceived;
            udpClient.onConnectionStatusChanged += onConnectionStatusChanged;
        }

        void Update()
        {
            /*
            if (currentFrame >= 403) currentFrame = 0; // Reset
            
            frameTimer += Time.deltaTime;
            
            if (frameTimer >= timeBetweenFrames)
            {
                frameTimer = 0f;
                
                string filePath = DATA_PATH + DATA_FILE + $"{currentFrame:D4}.txt";
                
                if (File.Exists(filePath))
                {
                    res = File.ReadAllText(filePath);

                    if (res.Length == 0)
                        Debug.LogWarning($"Empty Result Data for frame {currentFrame}.");
                    else
                        plotter.Update(res);
                }
                
                currentFrame++;
                
                if (currentFrame >= 403)
                    Debug.Log("Frame playback completed!");
            }*/
        }



        private void onDetectionDataReceived(string data)
        { }

        public void UpdateVisualization(string data)
        {
            // Debug.Log(data);
            if (!string.IsNullOrEmpty(data))
                plotter.Update(data);
        }
        
        private void onConnectionStatusChanged(bool connected)
        {
            Debug.Log(connected ? "Connected to Neural Daemon" : "Disconnected from Neural Daemon");
        }
    }
}
