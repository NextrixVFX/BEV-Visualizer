using System;
using System.Net;
using System.Net.Sockets;
using System.Threading;
using UnityEngine;
using System.Text;

namespace Visualizer.Networking
{
    public class UDPClient : MonoBehaviour
    {
        [Header("Config")]
        public string serverIP = "172.23.112.1";
        public int serverPort = 8080;
        public int clientPort = 8081;
        public bool autoConnect = true;

        [Header("Performance")]
        public int maxQueueSize = 490;

        private UdpClient udpClient;
        private Thread receiveThread;
        private bool isReceiving = false;  // Fixed spelling
        private System.Collections.Concurrent.ConcurrentQueue<string> messageQueue;

        // Events
        public Action<string> onMessageReceived;
        public Action<bool> onConnectionStatusChanged;

        private Initialize visualizer;

        void Start()
        {
            messageQueue = new System.Collections.Concurrent.ConcurrentQueue<string>();
            visualizer = FindObjectOfType<Initialize>();  // Moved from Connect()
            
            if (visualizer == null)
            {
                Debug.LogError("Initialize component not found in scene!");
                return;
            }

            if (autoConnect) 
                Connect();
        }

        public void Connect()
        {
            try
            {
                if (udpClient != null)
                {
                    Disconnect();
                }

                udpClient = new UdpClient(clientPort);
                udpClient.Client.ReceiveTimeout = 1000;

                isReceiving = true;
                receiveThread = new Thread(new ThreadStart(ReceiveData));
                receiveThread.IsBackground = true;  // Important: make it a background thread
                receiveThread.Start();

                onConnectionStatusChanged?.Invoke(true);
                Debug.Log($"UDP client started on port {clientPort}, connecting to {serverIP}:{serverPort}");
            }
            catch (Exception ex)
            {
                Debug.LogError($"Failed to start UDP client: {ex.Message}");
                onConnectionStatusChanged?.Invoke(false);
            }
        }

        public void Disconnect()
        {
            isReceiving = false;

            // Proper thread shutdown
            if (receiveThread != null && receiveThread.IsAlive)
            {
                receiveThread.Join(1000); // Wait 1 second for thread to finish
                if (receiveThread.IsAlive)
                {
                    receiveThread.Abort();
                }
                receiveThread = null;
            }

            udpClient?.Close();
            udpClient = null;

            messageQueue?.Clear();
            onConnectionStatusChanged?.Invoke(false);

            Debug.Log("UDP client disconnected");
        }

        private void ReceiveData()
        {
            IPEndPoint remoteEndPoint = new IPEndPoint(IPAddress.Any, 0);  // Fixed: listen to any IP

            while (isReceiving)
            {
                try
                {
                    byte[] data = udpClient.Receive(ref remoteEndPoint);
                    string message = Encoding.UTF8.GetString(data);
                    
                    // Add to queue (respect max size)
                    messageQueue.Enqueue(message);
                    while (messageQueue.Count > maxQueueSize)
                    {
                        messageQueue.TryDequeue(out _);
                    }
                    
                    onMessageReceived?.Invoke(message);
                }
                catch (SocketException e)
                {
                    if (e.SocketErrorCode == SocketError.Interrupted)
                    {
                        break; // Normal shutdown
                    }
                    else if (e.SocketErrorCode != SocketError.TimedOut)
                    {
                        Debug.LogWarning($"Waiting for server: {e.Message}");
                        Thread.Sleep(100); // Avoid tight loop on errors
                    }
                }
                catch (ThreadAbortException)
                {
                    break; // Normal thread termination
                }
                catch (Exception e)
                {
                    Debug.LogError($"UDP receive thread error: {e.Message}");
                    Thread.Sleep(100); // Avoid tight loop on errors
                }
            }
            
            Debug.Log("UDP receive thread stopped");
        }

        void Update()
        {
            // Process all queued messages
            int processed = 0;
            while (messageQueue != null && messageQueue.TryDequeue(out string data) && processed < 10) // Limit per frame
            {
                processed++;
                if (visualizer != null && !string.IsNullOrEmpty(data))
                {
                    visualizer.UpdateVisualization(data);
                }
            }
            
            // Optional: Display queue size occasionally
            if (Time.frameCount % 60 == 0 && messageQueue != null)
            {
                Debug.Log($"UDP queue size: {messageQueue.Count}");
            }
        }

        // Public method to send data back to server (if needed)
        public void SendData(string message)
        {
            if (udpClient == null || !isReceiving) return;

            try
            {
                byte[] data = Encoding.UTF8.GetBytes(message);
                IPEndPoint serverEndPoint = new IPEndPoint(IPAddress.Parse(serverIP), serverPort);
                udpClient.Send(data, data.Length, serverEndPoint);
            }
            catch (Exception e)
            {
                Debug.LogError($"Failed to send UDP data: {e.Message}");
            }
        }

        void OnApplicationQuit()
        {
            Disconnect();
        }

        void OnDestroy()
        {
            Disconnect();
        }

        // Editor helper to test connection
        [ContextMenu("Test Connection")]
        void TestConnection()
        {
            if (Application.isPlaying)
            {
                Connect();
            }
        }
    }
}