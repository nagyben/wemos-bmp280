data "google_project" "project" {
  depends_on = [google_project_service.enabled_apis]
}