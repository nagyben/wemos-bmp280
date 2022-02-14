resource "google_pubsub_topic" "pubsub_weather_topic" {
  name = "weather-topic${var.env_suffix}"
}