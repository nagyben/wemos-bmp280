output "receiver_url" {
    value = google_cloudfunctions_function.receiver_function.https_trigger_url
}