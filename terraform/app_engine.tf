resource "google_app_engine_application" "app_engine" {
  project     = var.project
  location_id = var.region
  database_type = "CLOUD_FIRESTORE"
}