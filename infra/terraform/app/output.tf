output "receiver_url" {
  value = google_cloudfunctions_function.receiver_function_authenticated.https_trigger_url
}

output "mqtt_tester_public_key" {
  value = length(google_service_account_key.mqtt_tester_key) > 0 ? base64decode(google_service_account_key.mqtt_tester_key[0].public_key) : null
}

output "topic_name" {
  value = google_pubsub_topic.pubsub_weather_topic.name
}

output "viz_url" {
  value = "http://${google_storage_bucket.static_site.name}"
}

output "static_site_bucket_name" {
  value = google_storage_bucket.static_site.name
}