resource "google_storage_bucket" "functions_storage_bucket" {
  name     = "nagyben-cloud-functions-source${var.env_suffix}"
  location = "EU"
}

resource "google_storage_bucket" "static_site" {
  name = "weather${var.env_suffix}.exantas.me"
  location = "EU"

  force_destroy = true

  website {
    main_page_suffix = "index.html"
    not_found_page   = "404.html"
  }
}

resource "google_storage_bucket_access_control" "public_rule" {
  bucket = google_storage_bucket.static_site.name
  role   = "READER"
  entity = "allUsers"
}