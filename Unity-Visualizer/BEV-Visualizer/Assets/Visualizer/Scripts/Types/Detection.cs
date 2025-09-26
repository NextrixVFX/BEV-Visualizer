using System;
using Unity.VisualScripting;
using UnityEngine;

namespace Visualizer.Types
{

    public class BoundingBox
    {
        public Vector3 position, scale;
        public Quaternion rotation;
        public float x, y, z, width, length, height, pivot;

        public BoundingBox(float x, float y, float z, float width, float length, float height, float pivot)
        {
            this.x = x;
            this.y = y;
            this.z = z;
            this.width = width;
            this.length = length;
            this.height = height;
            this.pivot = pivot;
            this.position = new Vector3(x, y, z);
            this.scale = new Vector3(width, height, length);
            this.rotation = Quaternion.Euler(0f, pivot * Mathf.Rad2Deg, 0f);
        }

        public BoundingBox(Vector3 position, Vector3 scale, Quaternion rotation)
        {
            this.x = position.x;
            this.y = position.y;
            this.z = position.z;
            this.width = scale.x;
            this.length = scale.z;
            this.height = scale.y;
            this.pivot = rotation.y;
            this.position = position;
            this.scale = scale;
            this.rotation = rotation;
        }
    }
    public class Detection
    {
        public int id;
        public float conf;
        public BoundingBox bbox;
        
        public Detection(float x, float y, float z, float width, float length, float height, float pivot, int id, float conf)
        {
            this.bbox = new(x,y * 0f + .5f,z,width,length,height,pivot);
            this.id = id;
            this.conf = conf;
        }

        public static Detection CreateFromString(string det)
        {
            if (string.IsNullOrWhiteSpace(det)) return null;
            
            string[] elements = det.Split(' ');
                
            float x = ValidateFloat(elements[0], "x");
            float y = ValidateFloat(elements[2], "y"); // Your flip yz to zy
            float z = ValidateFloat(elements[1], "z");
            float width = ValidateFloat(elements[3], "width");
            float length = ValidateFloat(elements[4], "length");
            float height = ValidateFloat(elements[5], "height");
            float pivot = ValidateFloat(elements[6], "pivot");
            int id = ValidateInt(elements[7], "id");
            float conf = ValidateFloat(elements[8], "confidence");

            return new Detection(x, y, z, width, length, height, pivot, id, conf);
        }

        private static float ValidateFloat(string value, string fieldName)
        {
            if (float.TryParse(value, out float result))
            {
                if (float.IsNaN(result) || float.IsInfinity(result))
                {
                    throw new FormatException($"Invalid {fieldName} value: {value}");
                }
                return result;
            }
            throw new FormatException($"Invalid {fieldName} format: {value}");
        }

        private static int ValidateInt(string value, string fieldName)
        {
            if (int.TryParse(value, out int result))
            {
                return result;
            }
            throw new FormatException($"Invalid {fieldName} format: {value}");
        }
    }
}