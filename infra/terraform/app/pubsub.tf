resource "google_pubsub_topic" "pubsub_weather_topic" {
  name = "weather-topic${var.env_suffix}"
}

resource "google_pubsub_subscription" "pubsub_weather_subscription" {
  name  = "weather-subscription${var.env_suffix}"
  topic = google_pubsub_topic.pubsub_weather_topic.name
}