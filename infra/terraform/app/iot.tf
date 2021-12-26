resource "google_cloudiot_registry" "cloudiot_registry" {
  name = "iot-registry"

  region = "europe-west1"

  event_notification_configs {
    pubsub_topic_name = google_pubsub_topic.pubsub_weather_topic.id
    subfolder_matches = ""
  }

}