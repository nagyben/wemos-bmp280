resource "google_pubsub_topic" "pubsub_weather_topic" {
  name = "weather-topic"
}

resource "google_pubsub_subscription" "pubsub_weather_subscription" {
  name  = "weather-subscription"
  topic = google_pubsub_topic.pubsub_weather_topic.name
}