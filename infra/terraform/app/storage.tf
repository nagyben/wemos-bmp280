resource "google_storage_bucket" "functions_storage_bucket" {
  name     = "nagyben-cloud-functions-source${var.env_suffix}"
  location = "EU"
}

resource "google_storage_bucket" "static_site" {
  name = "exantas.me/weather${var.env_suffix}"
  location = "EU"

  force_destroy = true

  uniform_bucket_level_access = true

  website {
    main_page_suffix = "index.html"
    not_found_page   = "404.html"
  }
}