resource "google_storage_bucket" "functions_storage_bucket" {
  name     = "nagyben-cloud-functions-source"
  location = "EU"
}