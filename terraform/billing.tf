data "google_billing_account" "account" {
  billing_account = "010695-569D68-655635"
  depends_on      = [google_project_service.enabled_apis]
}

resource "google_billing_budget" "budget" {
  billing_account = data.google_billing_account.account.id
  display_name    = "nagyben-budget"

  amount {
    specified_amount {
      currency_code = "GBP"
      units         = "20"
    }
  }

  threshold_rules {
    threshold_percent = 1.0
  }

  threshold_rules {
    threshold_percent = 1.0
    spend_basis       = "FORECASTED_SPEND"
  }

  all_updates_rule {
    monitoring_notification_channels = [
      google_monitoring_notification_channel.notification_channel.id,
    ]
    disable_default_iam_recipients = true
  }
}

resource "google_monitoring_notification_channel" "notification_channel" {
  display_name = "nagyben-gmail-com"
  type         = "email"

  labels = {
    email_address = var.billing_alert_email
  }
}