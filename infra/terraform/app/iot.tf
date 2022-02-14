resource "google_cloudiot_registry" "cloudiot_registry" {
  name = "iot-registry${var.env_suffix}"

  region = "europe-west1"

  event_notification_configs {
    pubsub_topic_name = google_pubsub_topic.pubsub_weather_topic.id
    subfolder_matches = ""
  }

}



resource "google_cloudiot_device" "iot_test_device" {
  count    = var.env_suffix == "" ? 0 : 1
  name     = "iot-test-device"
  registry = google_cloudiot_registry.cloudiot_registry.id

  credentials {
    public_key {
      format = "RSA_X509_PEM"
      key    = base64decode(google_service_account_key.mqtt_tester_key[0].public_key)
    }
  }

  blocked = false

  log_level = "INFO"

  gateway_config {
    gateway_type = "NON_GATEWAY"
  }
}