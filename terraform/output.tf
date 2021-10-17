output "receiver_url" {
  value = google_cloudfunctions_function.receiver_function_authenticated.https_trigger_url
}